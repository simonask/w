#include "recompiler.hpp"

#include <wayward/support/command_line_options.hpp>
#include <wayward/support/format.hpp>
#include <wayward/support/logger.hpp>
#include <wayward/support/http.hpp>

#include <iostream>
#include <cstdlib>

#include <event2/event.h>
#include <event2/http.h>
#include <event2/keyvalq_struct.h>
#include <event2/buffer.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>

extern "C" char** environ;

static void kill_all() {
  ::kill(0, SIGHUP);
}

static void usage(const std::string& program_name) {
  std::cout << wayward::format("Usage:\n\t{0} [app]\n\n", program_name);

  std::cout << "Options:\n";

  std::cout << "\t--port=<port>        Default: 3000\n";
  std::cout << "\t--address=<addr>     Default: 0.0.0.0\n";
  std::cout << "\t--no-live-recompile  Don't try to recompile the project on the fly\n";

  kill_all();
}

struct AppState {
  event_base* base = nullptr;
  evhttp_connection* connection_to_child = nullptr;
  std::string address = "0.0.0.0";
  int port = 3000;
  int child_server_fd = -1;
  int child_server_port = -1;

  pid_t child_server_pid = -1;
  pid_t process_group = -1;

  std::string directory;
  std::string binary_path;
  bool run_in_debugger = false;

  wayward::ConsoleStreamLogger logger = wayward::ConsoleStreamLogger{std::cout, std::cerr};
};

void respond_with_compilation_error(evhttp_request* req, const wayward::CompilationError& error) {
  evbuffer* buf = evbuffer_new();
  evbuffer_add_printf(buf, "Compilation Error:\n\n%s", error.what());
  evhttp_send_reply(req, 500, nullptr, buf);
  evbuffer_free(buf);
}

static void copy_headers(evkeyvalq* to, const evkeyvalq* from) {
  for (evkeyval* header = from->tqh_first; header; header = header->next.tqe_next) {
    evhttp_add_header(to, header->key, header->value);
  }
}

static void validate_child_server(AppState* state) {
  if (state->child_server_pid > 0) {
    // Re-validate child process (see if it's running):
    int r = ::kill(state->child_server_pid, 0);
    if (r != 0) {
      state->logger.log(wayward::Severity::Error, "w_dev", "App server has died.");
      state->child_server_pid = -1;
    }
  }
}

static bool rebuild_child_server_if_needed(AppState* state, evhttp_request* req) {
  wayward::Recompiler recompiler { state->directory };
  try {
    if (recompiler.needs_rebuild()) {
      state->logger.log(wayward::Severity::Information, "w_dev", wayward::format("Rebuilding '{0}'...", state->binary_path));
      recompiler.rebuild();
      if (state->child_server_pid > 0) {
        // Close any existing connections.
        if (state->connection_to_child != nullptr) {
          evhttp_connection_free(state->connection_to_child);
          state->connection_to_child = nullptr;
        }

        // Kill the child process.
        int r = ::kill(state->child_server_pid, 1);
        if (r != 0) {
          ::perror("kill");
        } else {
          state->child_server_pid = -1;
        }
      }
    }
    return true;
  }
  catch (const wayward::CompilationError& error) {
    state->logger.log(wayward::Severity::Error, "w_dev", error.what());
    respond_with_compilation_error(req, error);
    return false;
  }
}

static bool check_child_server_binary(AppState* state, evhttp_request* req) {
  struct stat bin_st;
  int r = ::stat(state->binary_path.c_str(), &bin_st);
  if (r == ENOENT) {
    auto error = wayward::CompilationError{"`make` did not generate the expected binary."};
    state->logger.log(wayward::Severity::Error, "w_dev", error.what());
    respond_with_compilation_error(req, error);
    return false;
  }
  return true;
}

static void spawn_child_server_if_needed(AppState* state) {
  if (state->child_server_pid < 0) {
    // Spawn the child server
    state->logger.log(wayward::Severity::Information, "w_dev", wayward::format("Spawning app server at port {0}...", state->child_server_port));

    state->child_server_pid = fork();
    if (state->child_server_pid == 0) {
      // We're the child!

      // Set the process group, so we get terminated when the parent terminates:
      int r = ::setpgid(0, state->process_group);
      if (r < 0) {
        ::perror("setpgid");
        kill_all();
      }

      // Replace self with the app server:
      std::string socketfd_str = wayward::format("{0}", state->child_server_fd);
      const char* args[] = {state->binary_path.c_str(), "--socketfd", socketfd_str.c_str(), "--die-when-orphaned"};
      r = ::execve(state->binary_path.c_str(), (char* const*)args, ::environ);
      ::perror("execve");
      kill_all();
    } else if (state->child_server_pid < 0) {
      ::perror("fork");
      kill_all();
    }
  }
}

static void child_connection_close_callback(evhttp_connection* conn, void* userdata) {
  AppState* state = (AppState*)userdata;
  if (state->connection_to_child == conn)
    state->connection_to_child = nullptr;
}

static void renew_child_connection(AppState* state) {
  // We cannot actually reuse the connection, because a reused connection causes EOF.
  // The memory gets freed automatically when whatever request gets completed and the connection is closed.
  unsigned short port = (unsigned short)state->child_server_port;
  const char* addr = "127.0.0.1";
  state->connection_to_child = evhttp_connection_base_new(state->base, nullptr, addr, port);
  evhttp_connection_set_closecb(state->connection_to_child, child_connection_close_callback, state);
  evhttp_connection_set_timeout(state->connection_to_child, 600);
}

static void child_request_callback(evhttp_request* child_request, void* userdata) {
  evhttp_request* client_request = (evhttp_request*)userdata;
  int        response_code    = evhttp_request_get_response_code(child_request);
  evkeyvalq* response_headers = evhttp_request_get_input_headers(child_request);
  evbuffer*  response_body    = evhttp_request_get_input_buffer(child_request);

  copy_headers(evhttp_request_get_output_headers(client_request), response_headers);

  evhttp_send_reply(client_request, response_code, nullptr, response_body);
}

static void child_request_error_callback(evhttp_request_error err, void* userdata) {
  using wayward::HTTPStatusCode;
  evhttp_request* client_request = (evhttp_request*)userdata;
  switch (err) {
    case EVREQ_HTTP_TIMEOUT: {
      evhttp_send_error(client_request, (int)HTTPStatusCode::GatewayTimeout, "Timeout from app server");;
      break;
    }
    case EVREQ_HTTP_EOF: {
      evhttp_send_error(client_request, (int)HTTPStatusCode::InternalServerError, "Unexpected EOF");
      break;
    }
    case EVREQ_HTTP_INVALID_HEADER: {
      evhttp_send_error(client_request, (int)HTTPStatusCode::BadRequest, "Invalid header");
      break;
    }
    case EVREQ_HTTP_BUFFER_ERROR: {
      evhttp_send_error(client_request, (int)HTTPStatusCode::InternalServerError, "Buffer error");
      break;
    }
    case EVREQ_HTTP_REQUEST_CANCEL: {
      evhttp_send_error(client_request, (int)HTTPStatusCode::InternalServerError, "Request Cancelled");
      break;
    }
    case EVREQ_HTTP_DATA_TOO_LONG: {
      evhttp_send_error(client_request, (int)HTTPStatusCode::RequestEntityTooLarge, nullptr);
      break;
    }
  }
}

static void forward_request_to_child(AppState* state, evhttp_request* req) {
  evhttp_request* child_req = evhttp_request_new(child_request_callback, req);
  evhttp_request_set_error_cb(child_req, child_request_error_callback);
  copy_headers(evhttp_request_get_output_headers(child_req), evhttp_request_get_input_headers(req));
  evbuffer_add_buffer(evhttp_request_get_output_buffer(child_req), evhttp_request_get_input_buffer(req));
  int r = evhttp_make_request(state->connection_to_child, child_req, evhttp_request_get_command(req), evhttp_request_get_uri(req));
  if (r != 0) {
    fprintf(stderr, "Error making request to child server.\n");
    kill_all();
  }
}

static void request_callback(evhttp_request* req, void* userdata) {
  AppState* state = (AppState*)userdata;
  validate_child_server(state);
  if (!rebuild_child_server_if_needed(state, req))
    return;
  if (!check_child_server_binary(state, req))
    return;
  spawn_child_server_if_needed(state);
  renew_child_connection(state);
  forward_request_to_child(state, req);
}

static std::vector<std::string> string_to_path_components(const std::string& path, const std::string& sep = "/") {
  std::vector<std::string> path_components;
  const char* p = path.c_str();
  while (*p) {
    const char* next = strstr(p, sep.c_str());
    if (next) {
      path_components.push_back(std::string(p, next - p));
      p = next+1;
    } else {
      path_components.push_back(p);
      break;
    }
  }
  return path_components;
}

static bool path_exists(const std::string& path) {
  struct stat s;
  return ::stat(path.c_str(), &s) == 0;
}

static bool path_is_directory(const std::string& path) {
  struct stat st;
  int r = ::stat(path.c_str(), &st);
  return r == 0 && S_ISDIR(st.st_mode);
}

static int make_child_server_socket(unsigned short child_server_port) {
  int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  int r;
  if (fd < 0) {
    ::perror("socket");
    kill_all();
  }
  r = evutil_make_socket_nonblocking(fd);
  if (r < 0) {
    ::perror("evutil_make_socket_nonblocking");
    kill_all();
  }
  r = evutil_make_listen_socket_reuseable(fd);
  if (r < 0) {
    ::perror("evutil_make_listen_socket_reuseable");
    kill_all();
  }
  int on = 1;
  r = ::setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void*)&on, sizeof(on));
  if (r < 0) {
    ::perror("setsockopt");
    kill_all();
  }
  struct sockaddr_in sa;
  sa.sin_family = AF_INET;
  sa.sin_port = htons(child_server_port);
  sa.sin_addr.s_addr = htonl(INADDR_ANY);
  r = ::bind(fd, (struct sockaddr*)&sa, sizeof(sa));
  if (r < 0) {
    ::perror("bind");
    kill_all();
  }
  r = ::listen(fd, 128);
  if (r < 0) {
    ::perror("listen");
    kill_all();
  }
  return fd;
}

static std::vector<std::string>
parse_command_line_options(int argc, char const* const* argv, AppState* state) {
  wayward::CommandLineOptions options;
  options.usage([&]() {
    usage(argv[0]);
  });

  options.description("Listen at port.");
  options.option("--port", "-p", [&](int64_t p) {
    state->port = p;
  });

  options.description("Listen at address.");
  options.option("--address", "-l", [&](const std::string& addr) {
    state->address = addr;
  });

  state->child_server_port = 54987;
  options.description("Spawn app server internally at port.");
  options.option("--internal-port", "-pp", [&](int64_t p) {
    state->child_server_port = p;
  });

  options.description("Run server under LLDB.");
  options.option("--debugger", "-d", [&]() {
    state->run_in_debugger = true;
  });

  return options.parse(argc, argv);
}

static AppState* g_state = nullptr;

static void exit_handler() {
  if (g_state && g_state->child_server_pid >= 0) {
    ::kill(g_state->child_server_pid, SIGHUP);
  }
}

namespace w_dev {
  int server(int argc, char const* const* argv) {
    using namespace wayward;
    AppState state;
    g_state = &state;
    auto args = parse_command_line_options(argc, argv, &state);

    // Check that first argument is a path to a binary.
    std::string path = args.size() >= 1 ? args.at(0) : std::string();
    auto path_components = string_to_path_components(path);

    if (path_components.empty()) {
      usage(argv[0]);
    }

    std::string directory;
    for (size_t i = 0; i < path_components.size()-1; ++i) {
      directory += path_components[i];
    }
    if (directory == "") {
      directory = ".";
    }

    if (!path_is_directory(directory)) {
      std::cerr << "Path must be a path to an app within a directory.\n";
      kill_all();
    }

    if (path_exists(path) && path_is_directory(path)) {
      std::cerr << "Path must be a path to an app binary.\n";
      kill_all();
    }

    state.directory = directory;
    state.binary_path = path;
    state.process_group = ::setpgrp();

    // Create the event loop:
    state.base = event_base_new();

    // Create the front-facing HTTP server:
    int r;
    evhttp* http = evhttp_new(state.base);
    evhttp_set_gencb(http, request_callback, &state);
    state.logger.log(Severity::Information, "w_dev", wayward::format("Dev server listening on {0}:{1}...", state.address, state.port));
    r = evhttp_bind_socket(http, state.address.c_str(), state.port);
    if (r != 0) {
      std::cerr << wayward::format("evhttp_bind_socket: {0}\n", ::strerror(errno));
      return 1;
    }

    // Create a listening socket for the backend:
    state.child_server_fd = make_child_server_socket(state.child_server_port);

    std::atexit(exit_handler);

    return event_base_dispatch(state.base);
  }
}

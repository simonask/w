#include "recompiler.hpp"

#include <wayward/support/command_line_options.hpp>
#include <wayward/support/format.hpp>
#include <wayward/support/logger.hpp>

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
  std::string directory;
  std::string binary_path;
  int internal_port;
  int child_server_fd = -1;
  wayward::ConsoleStreamLogger logger = wayward::ConsoleStreamLogger{std::cout, std::cerr};
  pid_t child_server = -1;
  pid_t process_group = -1;
  evhttp_connection* connection_to_child = nullptr;
};

void respond_with_compilation_error(evhttp_request* req, const wayward::CompilationError& error) {
  evbuffer* buf = evbuffer_new();
  evbuffer_add_printf(buf, "Compilation Error:\n\n%s", error.what());
  evhttp_send_reply(req, 500, nullptr, buf);
  evbuffer_free(buf);
}

static void copy_headers(const evkeyvalq* from, evkeyvalq* to) {
  for (evkeyval* header = from->tqh_first; header; header = header->next.tqe_next) {
    evhttp_add_header(to, header->key, header->value);
  }
}

static void child_request_callback(evhttp_request* req, void* userdata) {
  evhttp_request* client_request = (evhttp_request*)userdata;
  if (req) {
    int response_code = evhttp_request_get_response_code(req);
    evkeyvalq* response_headers = evhttp_request_get_input_headers(req);
    evbuffer*  response_body    = evhttp_request_get_input_buffer(req);

    copy_headers(response_headers, evhttp_request_get_output_headers(client_request));
    evhttp_send_reply(client_request, response_code, nullptr, response_body);
  } else {
    //evhttp_send_error(client_request, 500, "Error making request to app server");
  }
}

static void child_request_close_callback(evhttp_connection* conn, void* userdata) {
  AppState* state = (AppState*)userdata;
  state->connection_to_child = nullptr;
}

static void child_request_error_callback(evhttp_request_error err, void* userdata) {
  evhttp_request* client_request = (evhttp_request*)userdata;
  switch (err) {
    /**
     * Timeout reached, also @see evhttp_connection_set_timeout()
     */
    case EVREQ_HTTP_TIMEOUT: {
      evhttp_send_error(client_request, 500, "Timeout");;
      break;
    }
    /**
     * EOF reached
     */
    case EVREQ_HTTP_EOF: {
      evhttp_send_error(client_request, 500, "Unexpected EOF");
      break;
    }
    /**
     * Error while reading header, or invalid header
     */
    case EVREQ_HTTP_INVALID_HEADER: {
      evhttp_send_error(client_request, 500, "Invalid header");
      break;
    }
    /**
     * Error encountered while reading or writing
     */
    case EVREQ_HTTP_BUFFER_ERROR: {
      evhttp_send_error(client_request, 500, "Buffer error");
      break;
    }
    /**
     * The evhttp_cancel_request() called on this request.
     */
    case EVREQ_HTTP_REQUEST_CANCEL: {
      evhttp_send_error(client_request, 500, "Request Canceled");
      break;
    }
    /**
     * Body is greater then evhttp_connection_set_max_body_size()
     */
    case EVREQ_HTTP_DATA_TOO_LONG: {
      evhttp_send_error(client_request, 500, "Data Too Long");
      break;
    }
  }
}

static void renew_child_connection(AppState* state) {
  // We cannot actually reuse the connection, because a reused connection causes EOF.
  // The connection_to_child is set to NULL in the 'close' callback for the connection.
  if (state->connection_to_child != nullptr) {
    evhttp_connection_free(state->connection_to_child);
  }

  if (state->connection_to_child == nullptr) {
    state->connection_to_child = evhttp_connection_base_new(state->base, nullptr, "localhost", (short)state->internal_port);
    evhttp_connection_set_closecb(state->connection_to_child, child_request_close_callback, state);
    evhttp_connection_set_timeout(state->connection_to_child, 600);
  }
}

static void request_callback(evhttp_request* req, void* userdata) {
  using namespace wayward;

  AppState* state = (AppState*)userdata;
  Recompiler recompiler { state->directory };
  int r;


  if (state->child_server > 0) {
    // Re-validate child process (see if it's running):
    r = ::kill(state->child_server, 0);
    if (r < 0) {
      state->logger.log(Severity::Error, "w_dev", "App server has died.");
      state->child_server = -1;
    }
  }

  try {
    if (recompiler.needs_rebuild()) {
      state->logger.log(Severity::Information, "w_dev", wayward::format("Rebuilding '{0}'...", state->binary_path));
      recompiler.rebuild();
      if (state->child_server > 0) {
        r = ::kill(state->child_server, 1);
        if (r != 0) {
          ::perror("kill");
        } else {
          state->child_server = -1;
        }
      }
    }
  }
  catch (const CompilationError& error) {
    state->logger.log(Severity::Error, "w_dev", error.what());
    respond_with_compilation_error(req, error);
    return;
  }

  struct stat bin_st;
  r = ::stat(state->binary_path.c_str(), &bin_st);
  if (r == ENOENT) {
    auto error = CompilationError{"`make` did not generate the expected binary."};
    state->logger.log(Severity::Error, "w_dev", error.what());
    respond_with_compilation_error(req, error);
    return;
  }

  if (state->child_server < 0) {
    // Spawn the child server
    state->logger.log(Severity::Information, "w_dev", wayward::format("Spawning app server at port {0}...", state->internal_port));
    state->child_server = fork();
    if (state->child_server == 0) {
      // We're the child!

      // Set the process group, so we get terminated when the parent terminates:
      r = ::setpgid(0, state->process_group);
      if (r < 0) {
        ::perror("setpgid");
        kill_all();
      }

      // Replace self with the app server:
      std::string socketfd_str = wayward::format("{0}", state->child_server_fd);
      const char* args[] = {state->binary_path.c_str(), "--socketfd", socketfd_str.c_str()};
      r = ::execve(state->binary_path.c_str(), (char* const*)args, ::environ);
      ::perror("execve");
      kill_all();
    } else if (state->child_server < 0) {
      ::perror("fork");
      kill_all();
    }
  }

  // Okay we're good, pass the request on to the child server.
  renew_child_connection(state);
  evhttp_request* child_req = evhttp_request_new(child_request_callback, req);
  evhttp_request_set_error_cb(child_req, child_request_error_callback);
  copy_headers(evhttp_request_get_input_headers(req), evhttp_request_get_output_headers(child_req));
  evbuffer_add_buffer(evhttp_request_get_output_buffer(child_req), evhttp_request_get_input_buffer(req));
  r = evhttp_make_request(state->connection_to_child, child_req, evhttp_request_get_command(req), evhttp_request_get_uri(req));
  if (r < 0) {
    fprintf(stderr, "Error making request to child server.\n");
    kill_all();
  }
}

int main(int argc, char const *argv[])
{
  using namespace wayward;
  CommandLineOptions options {argc, argv};

  options.option("--help", "-h", [&]() {
    usage(argv[0]);
  });

  int port = 3000;
  options.option("--port", "-p", [&](int64_t p) {
    port = p;
  });

  std::string address = "0.0.0.0";
  options.option("--address", "-l", [&](const std::string& addr) {
    address = addr;
  });

  int internal_port = 54987;
  options.option("--internal-port", "-pp", [&](int64_t p) {
    internal_port = p;
  });

  // Check that first argument is a path to a binary.
  std::string path = argc >= 2 ? argv[1] : "";
  std::vector<std::string> path_components;
  const char* p = path.c_str();
  while (*p) {
    const char* next = strstr(p, "/");
    if (next) {
      path_components.push_back(std::string(p, next - p));
      p = next+1;
    } else {
      path_components.push_back(p);
      break;
    }
  }

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

  struct stat dir_st;
  ::stat(directory.c_str(), &dir_st);
  if (!S_ISDIR(dir_st.st_mode)) {
    std::cerr << "Path must be a path to an app within a directory.\n";
    kill_all();
  }

  struct stat bin_st;
  int r = ::stat(path.c_str(), &bin_st);
  if (r == 0) {
    if (S_ISDIR(bin_st.st_mode)) {
      std::cerr << "Path must be a path to an app binary.\n";
      kill_all();
    }
  }

  AppState state;
  state.directory = directory;
  state.binary_path = path;
  state.internal_port = internal_port;
  state.process_group = ::setpgrp();

  // Create the event loop:
  state.base = event_base_new();

  // Create the front-facing HTTP server:
  evhttp* http = evhttp_new(state.base);
  evhttp_set_gencb(http, request_callback, &state);
  state.logger.log(Severity::Information, "w_dev", wayward::format("Development server ready on {0}:{1}.", address, port));
  evhttp_bind_socket(http, address.c_str(), (short)port);

  // Create a listening socket for the backend:
  int fd = ::socket(AF_INET, SOCK_STREAM, 0);
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
  sa.sin_port = htons(internal_port);
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

  state.child_server_fd = fd;

  return event_base_dispatch(state.base);
}

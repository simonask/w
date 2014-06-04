#include "recompiler.hpp"

#include <wayward/support/command_line_options.hpp>
#include <wayward/support/format.hpp>
#include <wayward/support/logger.hpp>
#include <wayward/support/http.hpp>
#include <wayward/support/event_loop.hpp>
#include <wayward/support/fiber.hpp>

#include <iostream>
#include <cstdlib>
#include <cassert>

#include <event2/event.h>
#include <event2/util.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <arpa/inet.h>
#include <string.h>

extern "C" char** environ;

using wayward::Request;
using wayward::Response;
using wayward::HTTPServer;
using wayward::HTTPClient;
using wayward::HTTPStatusCode;
using wayward::EventLoop;

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
  EventLoop loop;
  std::string address = "0.0.0.0";
  int port = 3000;
  int child_server_fd = -1;
  int child_server_port = -1;

  pid_t child_server_pid = -1;
  pid_t process_group = -1;

  std::string directory;
  std::string binary_path;
  bool run_in_debugger = false;

  FILE* needs_rebuild_stream = nullptr;
  event* needs_rebuild_event = nullptr;
  bool needs_rebuild = false;

  std::shared_ptr<wayward::ILogger> logger;

  AppState() : logger{wayward::ConsoleStreamLogger::get()} {}
};

static Response response_for_error(AppState* state, std::exception_ptr exception) {
  Response response;
  response.code = HTTPStatusCode::InternalServerError;
  response.headers["Content-Type"] = "text/plain";
  try {
    std::rethrow_exception(exception);
  }
  catch (const wayward::CompilationError& error) {
    state->logger->log(wayward::Severity::Error, "w_dev", error.what());
    // TODO: Render a pretty template with the output from the compiler.
    response.body = wayward::format("Compilation Error: {0}", error.what());
  }
  catch (const std::exception& error) {
    state->logger->log(wayward::Severity::Error, "w_dev", error.what());
    response.body = wayward::format("Error: {0}", error.what());
  }
  catch (...) {
    response.body = "Unknown exception!";
  }
  return std::move(response);
}

static void validate_child_server(AppState* state) {
  if (state->child_server_pid > 0) {
    // Re-validate child process (see if it's running):
    int r = ::kill(state->child_server_pid, 0);
    if (r != 0) {
      state->logger->log(wayward::Severity::Error, "w_dev", "App server has died.");
      state->child_server_pid = -1;
    }
  }
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

static void in_directory(const std::string& dir, std::function<void()> action) {
  // TODO: Make exception-safe.
  int old_cwd = ::open(".", O_RDONLY);
  int new_cwd = ::open(dir.c_str(), O_RDONLY);
  ::fchdir(new_cwd);
  action();
  ::fchdir(old_cwd);
}

static void check_child_needs_rebuild_callback(int fd, short ev, void* userdata) {
  AppState* state = static_cast<AppState*>(userdata);

  // Pipe was closed
  event_free(state->needs_rebuild_event);
  state->needs_rebuild_event = nullptr;

  int result = ::pclose(state->needs_rebuild_stream);
  state->needs_rebuild_stream = nullptr;
  if (result != 0) {
    state->logger->log(wayward::Severity::Debug, "w_dev", "App needs rebuild (reason: target binary out of date).");
    state->needs_rebuild = true;
  }
}

static void check_child_needs_rebuild(AppState* state) {
  using namespace wayward;

  if (state->needs_rebuild)
    return;
  if (state->needs_rebuild_stream != nullptr)
    return;
  if (!path_exists(state->binary_path)) {
    state->logger->log(Severity::Debug, "w_dev", "App needs rebuild (reason: target binary does not exist).");
    state->needs_rebuild = true;
  }

  in_directory(state->directory, [&]() {
    state->needs_rebuild_stream = ::popen("scons -q > /dev/null", "r");
    int fd = fileno(state->needs_rebuild_stream);
    assert(fd > 0);
    event_base* base = (event_base*)state->loop.native_handle();
    state->needs_rebuild_event = event_new(base, fd, EV_READ, check_child_needs_rebuild_callback, state);
    event_add(state->needs_rebuild_event, nullptr);
  });
}

static void rebuild_child_server_if_needed(AppState* state) {
  if (state->needs_rebuild) {
    wayward::Recompiler recompiler { state->directory };
    state->logger->log(wayward::Severity::Information, "w_dev", wayward::format("Rebuilding '{0}'...", state->binary_path));
    recompiler.rebuild();
    if (state->child_server_pid > 0) {
      // Kill the child process.
      int r = ::kill(state->child_server_pid, 1);
      if (r != 0) {
        ::perror("kill");
      } else {
        state->child_server_pid = -1;
      }
    }
    state->needs_rebuild = false;
  }
}

static void check_child_server_binary(AppState* state) {
  struct stat bin_st;
  int r = ::stat(state->binary_path.c_str(), &bin_st);
  if (r == ENOENT) {
    throw wayward::CompilationError{"`scons` did not generate the expected binary."};
  }
}

static void spawn_child_server_if_needed(AppState* state) {
  if (state->child_server_pid < 0) {
    // Spawn the child server
    state->logger->log(wayward::Severity::Information, "w_dev", wayward::format("Spawning app server at port {0}...", state->child_server_port));

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

static Response forward_request_to_child(AppState* state, Request req) {
  wayward::HTTPClient client { "127.0.0.1", state->child_server_port };
  return client.request(std::move(req));
}

static wayward::Response request_callback(AppState* state, wayward::Request req) {
  try {
    validate_child_server(state);
    rebuild_child_server_if_needed(state);
    check_child_server_binary(state);
    spawn_child_server_if_needed(state);
    return forward_request_to_child(state, std::move(req));
  }
  catch (wayward::FiberTermination) {
    throw;
  }
  catch (...) {
    return response_for_error(state, std::current_exception());
  }
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
    using namespace wayward::units;

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

    // Create the front-facing HTTP server:
    HTTPServer http { state.address, state.port, [&](Request req) { return request_callback(&state, std::move(req)); } };
    try {
      http.start(&state.loop);
      state.logger->log(Severity::Information, "w_dev", wayward::format("Dev server listening on {0}:{1}...", state.address, state.port));
    }
    catch (const HTTPError& error) {
      state.logger->log(Severity::Error, "w_dev", wayward::format("HTTPError: {0}", error.what()));
      return 1;
    }

    // Create a listening socket for the backend:
    state.child_server_fd = make_child_server_socket(state.child_server_port);

    auto check_rebuild_task = state.loop.call_in(1_second, std::bind(check_child_needs_rebuild, &state), true);

    std::atexit(exit_handler);
    state.loop.run();
    return 0;
  }
}

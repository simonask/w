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

extern "C" char** environ;

static void usage(const std::string& program_name) {
  std::cout << wayward::format("Usage:\n\t{0} [app]\n\n", program_name);

  std::cout << "Options:\n";

  std::cout << "\t--port=<port>        Default: 3000\n";
  std::cout << "\t--address=<addr>     Default: 0.0.0.0\n";
  std::cout << "\t--no-live-recompile  Don't try to recompile the project on the fly\n";

  std::exit(1);
}

struct AppState {
  std::string directory;
  std::string binary_path;
  int internal_port;
  wayward::ConsoleStreamLogger logger = wayward::ConsoleStreamLogger{std::cout, std::cerr};
  pid_t child_server = -1;
};

void respond_with_compilation_error(evhttp_request* req, const wayward::CompilationError& error) {
  evbuffer* buf = evbuffer_new();
  evbuffer_add_printf(buf, "Compilation Error:\n\n%s", error.what());
  evhttp_send_reply(req, 500, nullptr, buf);
  evbuffer_free(buf);
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
          perror("kill");
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
      std::string port_str = wayward::format("{0}", state->internal_port);
      const char* args[] = {state->binary_path.c_str(), "--port", port_str.c_str()};
      r = ::execve(state->binary_path.c_str(), (char* const*)args, ::environ);
      perror("execve");
    } else if (state->child_server < 0) {
      perror("fork");
      exit(2);
    } else {
      // We're the parent!
      // TODO: Create the listening file descriptor in the parent and let the child take over, instead of this bullshit.
      ::usleep(500);
    }
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
    exit(2);
  }

  struct stat bin_st;
  int r = ::stat(path.c_str(), &bin_st);
  if (r == 0) {
    if (S_ISDIR(bin_st.st_mode)) {
    std::cerr << "Path must be a path to an app, not a directory.\n";
    exit(2);
    }
  }

  AppState state;
  state.directory = directory;
  state.binary_path = path;
  state.internal_port = port + 1;

  event_base* base = event_base_new();
  evhttp* http = evhttp_new(base);
  evhttp_set_gencb(http, request_callback, &state);
  state.logger.log(Severity::Information, "w_dev", wayward::format("Development server ready on {0}:{1}.", address, port));
  evhttp_bind_socket(http, address.c_str(), (short)port);

  return event_base_dispatch(base);
}

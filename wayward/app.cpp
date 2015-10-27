#include "wayward/w.hpp"
#include <wayward/support/datetime.hpp>
#include <wayward/support/command_line_options.hpp>
#include <wayward/support/event_loop.hpp>
#include <wayward/support/plugin.hpp>

#include <cxxabi.h>
#include <unistd.h>

#include <map>
#include <vector>
#include <sstream>
#include <iostream>
#include <regex>
#include <exception>

namespace wayward {
  using namespace wayward::units;

  namespace {
    struct Handler {
      std::string human_readable_regex;
      std::regex regex;
      std::string path;
      std::function<Response(Request&)> handler;
      std::map<size_t, std::string> regex_group_names;

      bool match_and_set_params(Request& req) const {
        MatchResults match;
        if (std::regex_match(req.uri.path, match, regex)) {
          for (auto& group_name: regex_group_names) {
            req.params[group_name.second] = std::string(match[group_name.first]);
          }
          return true;
        }
        return false;
      }
    };
  }

  struct App::Private {
    App* app = nullptr;
    std::string root;
    std::vector<std::pair<std::string, std::string>> asset_locations;
    std::map<std::string, std::vector<Handler>> method_handlers;

    EventLoop loop;
    std::unique_ptr<IEventHandle> die_when_orphaned_poll_event;

    std::string executable_path;
    std::string address = "0.0.0.0";
    int port = 3000;
    std::string environment = "development";
    Maybe<int> socket_from_parent_process;

    Handler handler_for_path(std::string path, std::function<Response(Request&)> callback) {
      Handler handler;
      handler.path = std::move(path);

      static const std::regex find_placeholder {"/:([\\w\\d]+)(/?)", std::regex::ECMAScript};
      static const std::string match_placeholder = "/([^/.]+)";
      std::stringstream rs;
      size_t group_counter = 1;
      regex_replace_stream(rs, handler.path, find_placeholder, [&](std::ostream& os, const MatchResults& match) {
        os << match_placeholder;
        handler.regex_group_names[group_counter++] = match[1];
        if (match.size() >= 2) {
          os << match[2]; // Trailing '/'
        }
      });

      // Trailing ".:format":
      rs << "(\\.([\\w\\d]+))?";
      group_counter++;
      handler.regex_group_names[group_counter++] = "format";

      handler.human_readable_regex = rs.str();
      handler.regex = std::regex(handler.human_readable_regex);
      handler.handler = std::move(callback);
      return std::move(handler);
    }

    void add_handler(std::string path, std::function<Response(Request&)> handler, std::string method) {
      auto it = method_handlers.find(method);
      if (it == method_handlers.end()) {
        auto pair = method_handlers.insert(std::make_pair(std::move(method), std::vector<Handler>()));
        it = pair.first;
      }
      auto& handlers = it->second;
      handlers.push_back(handler_for_path(std::move(path), handler));
    }

    Response respond_to_error(std::exception_ptr exception, const std::type_info* exception_type) {
      std::string type = demangle_symbol(exception_type->name());
      std::string what = "(unfamiliar exception type)";
      std::string backtrace = "(unsupported exception type)";

      try {
        std::rethrow_exception(exception);
      }
      catch (const wayward::Error& error) {
        what = error.what();
        std::stringstream backtrace_ss;
        for (auto& line: error.backtrace()) {
          backtrace_ss << line << "\n";
        }
        backtrace = backtrace_ss.str();
      }
      catch (const std::exception& error) {
        what = error.what();
      }
      catch (...) {
      }

      log::error("w", wayward::format("{0}: {1}\nBACKTRACE:\n{2}", type, what, backtrace));

      Response r;
      r.code = HTTPStatusCode::InternalServerError;
      r.headers["Content-Type"] = "text/plain";
      r.body = wayward::format("Internal Server Error\n\n{0}: {1}\n\nBacktrace:\n{2}", type, what, backtrace);
      return std::move(r);
    }

    Maybe<Response> respond_with_static_file(Request& req) {
      if (environment != "development")
        return Nothing; // Only serve static files in development.
      if (req.method != "GET")
        return Nothing;
      auto path = req.uri.path;
      if (path.find("..") != std::string::npos)
        return Nothing; // Never allow paths with '..'

      for (auto& pair: asset_locations) {
        if (path.find(pair.first) == 0) {
          std::string rem = path.substr(pair.first.size());
          if (rem.back() != '/')
            rem += '/';
          std::string local_path = pair.second + rem;
          return wayward::file(local_path);
        }
      }
      return Nothing;
    }

    Maybe<Response> respond_with_handler(Request& req) {
      auto handlers_it = method_handlers.find(req.method);
      Handler* h = nullptr;
      if (handlers_it != method_handlers.end()) {
        for (auto& handler: handlers_it->second) {
          if (handler.match_and_set_params(req)) {
            h = &handler;
          }
        }
      }

      if (h) {
        try {
          if (app->config.log_requests) {
            log::debug("w", wayward::format("Parameters: {0}", as_json(req.params, JSONMode::Compact)));
          }
          return h->handler(req);
        }
        catch (Response thrown_response) {
          return std::move(thrown_response);
        }
        catch (...) {
          return respond_to_error(std::current_exception(), __cxxabiv1::__cxa_current_exception_type());
        }
      }

      return Nothing;
    }

    Response respond_to_request(Request req) {
      auto t0 = DateTime::now();

      if (app->config.log_requests) {
        std::cout << "\n";
        log::debug("w", wayward::format("Starting {0} {1}...", req.method, req.uri.path));
      }

      Maybe<Response> response = Nothing;

      response = respond_with_static_file(req);

      if (!response) {
        response = respond_with_handler(req);
      }

      if (!response) {
        response = wayward::not_found();
      }

      auto t1 = DateTime::now();
      if (app->config.log_requests) {
        auto time_elapsed = t1 - t0;
        double us = time_elapsed.microseconds().repr_.count();
        double ms = us / 1000.0;
        log::info("w", wayward::format("Finished {0} {1} with {2} in {3} ms", req.method, req.uri.path, (int)response->code, ms));
      }

      return std::move(*response);
    }
  };

  static void app_die_if_orphaned() {
    // If the parent process becomes 1, we have been orphaned.
    pid_t parent_pid = ::getppid();
    if (parent_pid == 1) {
      w::log::error("App server orphaned, exiting.");
      ::exit(1);
    }
  }

  App::App(int argc, char const* const* argv) : priv(new Private) {
    priv->app = this;
    priv->executable_path = argv[0];

    CommandLineOptions cmd;

    cmd.description("Set the app server environment (development, production, staging).");
    cmd.option("--environment", "-e", [&](const std::string& env) {
      priv->environment = env;
    });

    cmd.description("The app server port.");
    cmd.option("--port", "-p", [&](int64_t port) {
      priv->port = port;
    });

    cmd.description("Listen address.");
    cmd.option("--address", [&](const std::string& address) {
      priv->address = address;
    });

    cmd.description("Use a listening socket passed down by a parent process.");
    cmd.option("--socketfd", [&](int64_t sockfd) {
      priv->socket_from_parent_process = (int)sockfd;
    });

    cmd.description("Periodically check if our parent process has died, and die with it.");
    cmd.option("--die-when-orphaned", [&]() {
      priv->die_when_orphaned_poll_event = priv->loop.call_in(100_milliseconds, app_die_if_orphaned, true);
    });

    cmd.description("Enable or disable access log (values: on, off).");
    cmd.option("--access-log", [&](std::string option) {
      if (option == "on") {
        config.log_requests = true;
      } else if (option == "off") {
        config.log_requests = false;
      }
    });

    cmd.usage("--help", "-h");
    cmd.parse(argc, argv);


    // Find absolute root:
    std::string program_name = argv[0];
    auto last_sep = program_name.find_last_of('/');
    std::string path = last_sep == std::string::npos ? std::string(".") : program_name.substr(0, last_sep);
    char* real_path = ::realpath(path.c_str(), nullptr);
    priv->root = std::string(real_path);
    ::free(real_path);

    cmd.option("--root", [&](const std::string& root) {
      priv->root = root;
    });
  }

  App::~App() {}

  Response App::request(Request req) {
    return priv->respond_to_request(std::move(req));
  }

  int App::run() {
    std::unique_ptr<HTTPServer> server;
    std::function<Response(Request)> handler = std::bind(&App::request, this, std::placeholders::_1);
    if (priv->socket_from_parent_process) {
      server = std::unique_ptr<HTTPServer>(new HTTPServer(*priv->socket_from_parent_process, std::move(handler)));
    } else {
      server = std::unique_ptr<HTTPServer>(new HTTPServer(priv->address, priv->port, std::move(handler)));
      log::info("w", wayward::format("Listening for connections on {0}:{1}", priv->address, priv->port));
    }

    server->start(&priv->loop);
    priv->loop.run();
    return 0;
  }

  void App::add_route(std::string method, std::string path, std::function<Response(Request&)> handler) {
    priv->add_handler(std::move(path), std::move(handler), std::move(method));
  }

  void App::assets(std::string uri_path, std::string filesystem_path) {
    priv->asset_locations.push_back({std::move(uri_path), std::move(filesystem_path)});
  }

  void App::print_routes() const {
    for (auto& method_handlers: priv->method_handlers) {
      for (auto& handler: method_handlers.second) {
        std::cout << method_handlers.first << " " << handler.path << "    " << handler.human_readable_regex << "\n";
      }
    }
  }

  std::string App::root() const {
    return priv->root;
  }
}

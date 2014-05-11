#include "wayward/w.hpp"
#include "wayward/private.hpp"
#include <wayward/support/datetime.hpp>
#include <wayward/support/command_line_options.hpp>

#include <event2/event.h>
#include <event2/http.h>
#include <event2/keyvalq_struct.h>
#include <event2/buffer.h>

#include <cxxabi.h>

#include <map>
#include <vector>
#include <sstream>
#include <iostream>
#include <regex>
#include <exception>

namespace wayward {
  namespace {
    struct Handler {
      std::string human_readable_regex;
      std::regex regex;
      std::string path;
      std::function<Response(Request&)> handler;
      std::map<size_t, std::string> regex_group_names;

      bool match_and_set_params(Request& req) const {
        MatchResults match;
        if (std::regex_match(req.uri.path(), match, regex)) {
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
    event_base* base = nullptr;
    evhttp* http = nullptr;

    std::string executable_path;
    CommandLineOptions command_line_options;
    std::string address = "0.0.0.0";
    int port = 3000;
    std::string environment = "development";
    Maybe<int> socket_from_parent_process;

    Handler handler_for_path(std::string path, std::function<Response(Request&)> callback) {
      Handler handler;
      static const std::regex escape {"[.$|()[\\]*+?/\\\\^]"};
      static const std::string replacement = "\\\\\\1&";
      std::string escaped_path = std::regex_replace(path, escape, replacement);
      handler.path = std::move(path);

      static const std::regex find_placeholder {"/\\:([a-zA-Z0-9]+)(/?)"};
      static const std::string match_placeholder = "/(.+)";
      std::stringstream rs;
      size_t group_counter = 1;
      regex_replace_stream(rs, handler.path, find_placeholder, [&](std::ostream& os, const MatchResults& match) {
        os << match_placeholder;
        handler.regex_group_names[group_counter++] = match[1];
        if (match.size() >= 2) {
          os << match[2]; // Trailing '/'
        }
      });

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

    void handle_request(evhttp_request* req) {
      Request r = priv::make_request_from_evhttp_request(req);
      respond_to_request(r, req);
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

    Maybe<Response> respond_with_static_file(Request& req, evhttp_request* handle) {
      if (environment != "development")
        return Nothing; // Only serve static files in development.
      if (req.method != "GET")
        return Nothing;
      auto path = req.uri.path();
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

    Maybe<Response> respond_with_handler(Request& req, evhttp_request* handle) {
      auto handlers_it = method_handlers.find(req.method);
      Handler* h = nullptr;
      if (handlers_it != method_handlers.end()) {
        for (auto& handler: handlers_it->second) {
          if (handler.match_and_set_params(req)) {
            h = &handler;
          }
        }
      }

      Response response;
      if (h) {
        try {
          return h->handler(req);
        } catch (...) {
          return respond_to_error(std::current_exception(), __cxxabiv1::__cxa_current_exception_type());
        }
      }

      return Nothing;
    }

    void send_response(Response& response, evhttp_request* handle) {
      evkeyvalq* headers = evhttp_request_get_output_headers(handle);
      for (auto& pair: response.headers) {
        evhttp_add_header(headers, pair.first.c_str(), pair.second.c_str());
      }

      evbuffer* body_buffer = evbuffer_new();
      evbuffer_add(body_buffer, response.body.c_str(), response.body.size());
      evhttp_send_reply(handle, (int)response.code, response.reason.size() ? response.reason.c_str() : nullptr, body_buffer);
      evbuffer_free(body_buffer);
    }

    void respond_to_request(Request& req, evhttp_request* handle) {
      auto t0 = DateTime::now();

      if (app->config.log_requests) {
        log::debug("w", wayward::format("Starting {0} {1}", req.method, req.uri.path()));
      }

      Maybe<Response> response = Nothing;

      response = respond_with_static_file(req, handle);

      if (!response) {
        response = respond_with_handler(req, handle);
      }

      if (!response) {
        response = wayward::not_found();
      }

      send_response(*response, handle);

      auto t1 = DateTime::now();
      if (app->config.log_requests) {
        auto time_elapsed = t1 - t0;
        double us = time_elapsed.microseconds().repr_.count();
        double ms = us / 1000.0;
        log::info("w", wayward::format("Finished {0} {1} with {2} in {3} ms", req.method, req.uri.path(), (int)response->code, ms));
      }
    }
  };

  static void app_http_request_cb(evhttp_request* req, void* userdata) {
    App* app = static_cast<App*>(userdata);
    app->priv->handle_request(req);
  }

  App::App(int argc, char const* const* argv) : priv(new Private) {
    priv->app = this;
    priv->base = event_base_new();
    priv->http = evhttp_new(priv->base);
    evhttp_set_gencb(priv->http, app_http_request_cb, this);

    priv->executable_path = argv[0];
    priv->command_line_options.set(argc, argv);

    auto& cmd = priv->command_line_options;

    cmd.option("--environment", "-e", [&](const std::string& env) {
      priv->environment = env;
    });

    cmd.option("--port", "-p", [&](int64_t port) {
      priv->port = port;
    });

    cmd.option("--address", [&](const std::string& address) {
      priv->address = address;
    });

    cmd.option("--socketfd", [&](int64_t sockfd) {
      priv->socket_from_parent_process = (int)sockfd;
    });

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

  App::~App() {
    evhttp_free(priv->http);
    event_base_free(priv->base);
  }

  int App::run() {
    int fd;
    if (priv->socket_from_parent_process) {
      fd = *priv->socket_from_parent_process;
      int r = evhttp_accept_socket(priv->http, fd);
      if (r < 0) {
        log::error("w", wayward::format("Could not listen on provided socket {0}.", fd));
        return 1;
      }
    } else {
      fd = evhttp_bind_socket(priv->http, priv->address.c_str(), (u_short)priv->port);
      if (fd < 0) {
        log::error("w", "Could not bind to socket.");
        return 1;
      }
      log::info("w", wayward::format("Listening for connections on {0}:{1}", priv->address, priv->port));
    }

    return event_base_dispatch(priv->base);
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

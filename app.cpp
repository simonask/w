#include "w.hpp"
#include "private.hpp"
#include <map>
#include <vector>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/keyvalq_struct.h>
#include <event2/buffer.h>
#include <sstream>
#include <iostream>

namespace w {
  namespace {
    struct Handler {
      std::string path;
      std::function<Response(Request&)> handler;
    };
  }

  struct App::Private {
    App* app = nullptr;
    std::map<std::string, std::vector<Handler>> method_handlers;
    event_base* base = nullptr;
    evhttp* http = nullptr;

    void add_handler(std::string path, std::function<Response(Request&)> handler, std::string method) {
      auto it = method_handlers.find(method);
      if (it == method_handlers.end()) {
        auto pair = method_handlers.insert(std::make_pair(std::move(method), std::vector<Handler>()));
        it = pair.first;
      }
      auto& handlers = it->second;
      handlers.push_back({std::move(path), std::move(handler)});
    }

    void handle_request(evhttp_request* req) {
      Request r = priv::make_request_from_evhttp_request(req);
      respond_to_request(r, req);
    }

    void respond_to_request(Request& req, evhttp_request* handle) {
      auto handlers_it = method_handlers.find(req.method);
      Handler* h = nullptr;
      if (handlers_it != method_handlers.end()) {
        for (auto& handler: handlers_it->second) {
          if (handler.path == req.uri.path()) {
            h = &handler;
          }
        }
      }

      Response response;
      if (h) {
        response = h->handler(req);
      } else {
        response.code = HTTPStatusCode::NotFound;
        response.body = "Not Found";
        response.headers["Content-Type"] = "text/plain";
      }

      evkeyvalq* headers = evhttp_request_get_output_headers(handle);
      for (auto& pair: response.headers) {
        evhttp_add_header(headers, pair.first.c_str(), pair.second.c_str());
      }

      if (app->config.log_requests) {
        std::cout << req.method << " " << req.uri.path() << " " << (int)response.code << '\n';
      }

      evbuffer* body_buffer = evbuffer_new();
      evbuffer_add(body_buffer, response.body.c_str(), response.body.size());
      evhttp_send_reply(handle, (int)response.code, response.reason.size() ? response.reason.c_str() : nullptr, body_buffer);
      evbuffer_free(body_buffer);
    }
  };

  App::App() : priv(new Private) { priv->app = this; }

  App::~App() {
    evhttp_free(priv->http);
    event_base_free(priv->base);
  }

  static void app_http_request_cb(evhttp_request* req, void* userdata) {
    App* app = static_cast<App*>(userdata);
    app->priv->handle_request(req);
  }

  int App::listen_and_serve(std::string address, int port) {
    priv->base = event_base_new();
    priv->http = evhttp_new(priv->base);
    evhttp_set_gencb(priv->http, app_http_request_cb, this);
    int fd = evhttp_bind_socket(priv->http, address.c_str(), (u_short)port);
    if (fd < 0) return fd;
    return event_base_dispatch(priv->base);
  }

  void App::get(std::string path, std::function<Response(Request&)> handler) {
    priv->add_handler(std::move(path), std::move(handler), "GET");
  }

  void App::put(std::string path, std::function<Response(Request&)> handler) {
    priv->add_handler(std::move(path), std::move(handler), "PUT");
  }

  void App::post(std::string path, std::function<Response(Request&)> handler) {
    priv->add_handler(std::move(path), std::move(handler), "POST");
  }

  void App::del(std::string path, std::function<Response(Request&)> handler) {
    priv->add_handler(std::move(path), std::move(handler), "DELETE");
  }

  void App::head(std::string path, std::function<Response(Request&)> handler) {
    priv->add_handler(std::move(path), std::move(handler), "HEAD");
  }

  void App::options(std::string path, std::function<Response(Request&)> handler) {
    priv->add_handler(std::move(path), std::move(handler), "OPTIONS");
  }
}

#include <wayward/support/http.hpp>
#include <wayward/support/event_loop.hpp>

#include <cassert>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/keyvalq_struct.h>

namespace wayward {
  namespace {
    Request make_request_from_evhttp_request(evhttp_request* req) {
      Request r;

      switch (evhttp_request_get_command(req)) {
        case EVHTTP_REQ_GET:     r.method = "GET";     break;
        case EVHTTP_REQ_POST:    r.method = "POST";    break;
        case EVHTTP_REQ_HEAD:    r.method = "HEAD";    break;
        case EVHTTP_REQ_PUT:     r.method = "PUT";     break;
        case EVHTTP_REQ_DELETE:  r.method = "DELETE";  break;
        case EVHTTP_REQ_OPTIONS: r.method = "OPTIONS"; break;
        case EVHTTP_REQ_TRACE:   r.method = "TRACE";   break;
        case EVHTTP_REQ_CONNECT: r.method = "CONNECT"; break;
        case EVHTTP_REQ_PATCH:   r.method = "PATCH";   break;
      }

      evkeyvalq* headers = evhttp_request_get_input_headers(req);
      for (evkeyval* header = headers->tqh_first; header; header = header->next.tqe_next) {
        r.headers[header->key] = header->value;
      }

      evbuffer* body = evhttp_request_get_input_buffer(req);
      if (body) {
        std::stringstream body_buffer;
        while (evbuffer_get_length(body)) {
          char tmp[128];
          int n = evbuffer_remove(body, tmp, sizeof(tmp));
          if (n > 0)
            body_buffer.write(tmp, n);
        }
        r.body = body_buffer.str();
      }

      char* decoded_uri_str = evhttp_decode_uri(evhttp_request_get_uri(req));
      r.uri = URI(decoded_uri_str);
      ::free(decoded_uri_str);
      return r;
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
  }

  struct HTTPServer::Private {
    evhttp* http = nullptr;
    std::function<Response(Request)> handler;

    int socket_fd = -1;
    std::string listen_host;
    int port = -1;
  };

  HTTPServer::HTTPServer(int socket_fd, std::function<Response(Request)> handler) : p_(new Private) {
    p_->socket_fd = socket_fd;
    p_->handler = std::move(handler);
  }

  HTTPServer::HTTPServer(std::string listen_host, int port, std::function<Response(Request)> handler) : p_(new Private) {
    p_->listen_host = std::move(listen_host);
    p_->port = port;
    p_->handler = std::move(handler);
  }

  HTTPServer::~HTTPServer() {
    stop();
  }

  namespace {
    static void http_server_callback(evhttp_request* req, void* userdata) {
      auto p = static_cast<HTTPServer::Private*>(userdata);
      auto request = make_request_from_evhttp_request(req);
      auto response = p->handler(std::move(request));
      send_response(response, req);
    }
  }

  void HTTPServer::start(IEventLoop* loop) {
    assert(p_->http == nullptr);
    event_base* base = (event_base*)loop->native_handle();
    p_->http = evhttp_new(base);
    evhttp_set_gencb(p_->http, http_server_callback, p_.get());
    if (p_->socket_fd >= 0) {
      int r = evhttp_accept_socket(p_->http, p_->socket_fd);
      if (r < 0) {
        throw HTTPError(wayward::format("Could not listen on provided socket: {0}.", p_->socket_fd));
      }
    } else {
      int r = evhttp_bind_socket(p_->http, p_->listen_host.c_str(), p_->port);
      if (r < 0) {
        throw HTTPError(wayward::format("Could not bind to socket on {0}:{1}.", p_->listen_host, p_->port));
      }
    }
  }

  void HTTPServer::stop() {
    // XXX: Some way to check if requests are being served?
    evhttp_free(p_->http);
    p_->http = nullptr;
  }

}

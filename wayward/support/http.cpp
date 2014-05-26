#include <wayward/support/http.hpp>
#include <wayward/support/event_loop.hpp>
#include <wayward/support/fiber.hpp>
#include <wayward/support/teamwork.hpp>

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

    Response make_response_from_evhttp_request(evhttp_request* req) {
      Response r;
      r.code = (HTTPStatusCode)evhttp_request_get_response_code(req);

      evkeyvalq* headers = evhttp_request_get_input_headers(req);
      for (evkeyval* header = headers->tqh_first; header; header = header->next.tqe_next) {
        r.headers[header->key] = header->value;
      }

      evbuffer* body = evhttp_request_get_input_buffer(req);
      // TODO: Insert length sanity checks?
      size_t body_len = evbuffer_get_length(body);
      r.body.resize(body_len);
      evbuffer_copyout(body, &r.body[0], body_len);
      return std::move(r);
    }

    void add_evhttp_request_headers_and_body(evhttp_request* req, const Request& r) {
      evkeyvalq* headers = evhttp_request_get_output_headers(req);
      for (auto& pair: r.headers) {
        evhttp_add_header(headers, pair.first.c_str(), pair.second.c_str());
      }

      if (r.body.size()) {
        evbuffer* body = evhttp_request_get_output_buffer(req);
        evbuffer_add(body, r.body.data(), r.body.size());
      }
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
    Teamwork team;

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
      p->team.work([=]() {
        auto caller = fiber::current();
        fiber::start([=]() {
          auto request = make_request_from_evhttp_request(req);
          auto response = p->handler(std::move(request));
          send_response(response, req);
        });
      });
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



  struct HTTPClient::Private {
    IEventLoop* loop = nullptr;
    evhttp_connection* conn = nullptr;
    FiberPtr initiating_fiber;
    Maybe<Response> recorded_response;
    std::exception_ptr error;
  };

  HTTPClient::HTTPClient(std::string host, int port) : p_(new Private) {
    p_->loop = current_event_loop();
    if (p_->loop == nullptr) {
      throw HTTPError("Cannot use HTTPClient outside of an event loop.");
    }

    event_base* base = static_cast<event_base*>(p_->loop->native_handle());
    // TODO: support evdns_base as well
    p_->conn = evhttp_connection_base_new(base, nullptr, host.c_str(), port);
    if (p_->conn == nullptr) {
      throw HTTPError(wayward::format("Could not connect to {0}:{1}.", host, port));
    }
  }

  HTTPClient::~HTTPClient() {}

  std::string HTTPClient::host() const {
    char* address;
    ev_uint16_t port;
    evhttp_connection_get_peer(p_->conn, &address, &port);
    return address;
  }

  int HTTPClient::port() const {
    char* address;
    ev_uint16_t port;
    evhttp_connection_get_peer(p_->conn, &address, &port);
    return port;
  }

  namespace {
    static void http_client_callback(evhttp_request* req, void* userdata) {
      HTTPClient* client = static_cast<HTTPClient*>(userdata);
      client->p_->recorded_response = make_response_from_evhttp_request(req);
      fiber::resume(std::move(client->p_->initiating_fiber));
    }

    static void http_client_error_callback(evhttp_request_error err, void* userdata) {
      HTTPClient* client = static_cast<HTTPClient*>(userdata);
      client->p_->recorded_response = Nothing;
      try {
        std::string message;
        switch (err) {
          case EVREQ_HTTP_TIMEOUT: {
            message = "Timeout";
            break;
          }
          case EVREQ_HTTP_EOF: {
            message = "Unexpected EOF";
            break;
          }
          case EVREQ_HTTP_INVALID_HEADER: {
            message = "Invalid header";
            break;
          }
          case EVREQ_HTTP_BUFFER_ERROR: {
            message = "Buffer error";
            break;
          }
          case EVREQ_HTTP_REQUEST_CANCEL: {
            message = "Request Cancelled";
            break;
          }
          case EVREQ_HTTP_DATA_TOO_LONG: {
            message = "Data Too Long";
            break;
          }
        }
        throw HTTPError{message};
      } catch (...) {
        client->p_->error = std::current_exception();
      }
      fiber::resume(std::move(client->p_->initiating_fiber));
    }
  }

  Response HTTPClient::request(Request req) {
    assert(p_->conn);
    evhttp_request* r = evhttp_request_new(http_client_callback, this);
    evhttp_request_set_error_cb(r, http_client_error_callback);
    evhttp_cmd_type cmd;
    if (req.method == "GET") {
      cmd = EVHTTP_REQ_GET;
    } else if (req.method == "POST") {
      cmd = EVHTTP_REQ_POST;
    } else if (req.method == "HEAD") {
      cmd = EVHTTP_REQ_HEAD;
    } else if (req.method == "PUT") {
      cmd = EVHTTP_REQ_PUT;
    } else if (req.method == "DELETE") {
      cmd = EVHTTP_REQ_DELETE;
    } else if (req.method == "OPTIONS") {
      cmd = EVHTTP_REQ_OPTIONS;
    } else if (req.method == "TRACE") {
      cmd = EVHTTP_REQ_TRACE;
    } else if (req.method == "CONNECT") {
      cmd = EVHTTP_REQ_CONNECT;
    } else if (req.method == "PATCH") {
      cmd = EVHTTP_REQ_PATCH;
    } else {
      throw HTTPError(wayward::format("Invalid method: {0}", req.method));
    }
    p_->initiating_fiber = fiber::current();
    p_->recorded_response = Nothing;
    p_->error = nullptr;
    int result = evhttp_make_request(p_->conn, r, cmd, req.uri.path().c_str());
    if (result < 0) {
      throw HTTPError("Error making HTTP request.");
    }

    // Resume the event loop, presumably.
    fiber::yield();

    if (p_->error) {
      std::rethrow_exception(p_->error);
    }

    if (!p_->recorded_response) {
      throw HTTPError("Fiber originating the HTTP request was resumed by something other than the completion callback.");
    }
    return std::move(*p_->recorded_response);
  }

  Response HTTPClient::request(std::string method, std::string path, Maybe<std::string> body, Params params, Headers headers) {
    Request req;
    req.method = std::move(method);
    req.uri = URI(std::move(path));
    req.params = std::move(params);
    req.headers = std::move(headers);
    if (body) {
      req.body = std::move(*body);
    }
    return request(std::move(req));
  }

  Response HTTPClient::get(std::string path, Params params, Headers headers) {
    return request("GET", std::move(path), Nothing, std::move(params), std::move(headers));
  }

  Response HTTPClient::post(std::string path, Params params, Headers headers) {
    return request("POST", std::move(path), Nothing, std::move(params), std::move(headers));
  }

  Response HTTPClient::post(std::string path, std::string body, Params params, Headers headers) {
    return request("POST", std::move(path), std::move(body), std::move(params), std::move(headers));
  }

  Response HTTPClient::put(std::string path, Params params, Headers headers) {
    return request("PUT", std::move(path), Nothing, std::move(params), std::move(headers));
  }

  Response HTTPClient::put(std::string path, std::string body, Params params, Headers headers) {
    return request("PUT", std::move(path), std::move(body), std::move(params), std::move(headers));
  }

  Response HTTPClient::patch(std::string path, Params params, Headers headers) {
    return request("PATCH", std::move(path), Nothing, std::move(params), std::move(headers));
  }

  Response HTTPClient::patch(std::string path, std::string body, Params params, Headers headers) {
    return request("PATCH", std::move(path), std::move(body), std::move(params), std::move(headers));
  }

  Response HTTPClient::del(std::string path, Params params, Headers headers) {
    return request("DELETE", std::move(path), Nothing, std::move(params), std::move(headers));
  }

  Response HTTPClient::options(std::string path, Params params, Headers headers) {
    return request("OPTIONS", std::move(path), Nothing, std::move(params), std::move(headers));
  }

  Response HTTPClient::head(std::string path, Params params, Headers headers) {
    return request("HEAD", std::move(path), Nothing, std::move(params), std::move(headers));
  }

}

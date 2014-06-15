#include <wayward/support/http.hpp>
#include <wayward/support/event_loop.hpp>
#include <wayward/support/fiber.hpp>
#include <wayward/support/event_loop_private.hpp>
#include <wayward/support/string.hpp>

#include <cassert>
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/keyvalq_struct.h>
#include <evhtp.h>

#include <mutex>

namespace wayward {
  namespace {
    std::vector<std::string> get_keys_from_destructured_param(const std::string& k) {
      // k must be well-formed at this point.
      std::vector<std::string> keys;
      auto bracket_pos = k.find('[');
      if (bracket_pos != std::string::npos) {
        keys.push_back(k.substr(0, bracket_pos));
        while (bracket_pos != std::string::npos) {
          auto end_bracket_pos = k.find(']', bracket_pos);
          keys.push_back(k.substr(bracket_pos + 1, end_bracket_pos - bracket_pos - 1));
          bracket_pos = k.find('[', end_bracket_pos);
        }
      } else {
        keys.push_back(k);
      }
      return std::move(keys);
    }

    void add_destructured_param_to_dict(data_franca::Object& params, std::string k, std::string v) {
      static const std::regex restructuralize_dictionary_keys { "^([^\\[]+)(\\[[^\\]]+\\])*$" };
      MatchResults results;
      if (std::regex_match(k, results, restructuralize_dictionary_keys)) {
        auto keys = get_keys_from_destructured_param(k);
        data_franca::Object* dict = &params;
        for (size_t i = 0; i < keys.size(); ++i) {
          auto& key = keys[i];

          if (i + 1 == keys.size()) {
            (*dict)[key] = std::move(v);
          } else {
            dict = &(*dict)[key];
          }
        }
      } else {
        params[k] = std::move(v);
      }
    }

    Request make_request_from_evhttp_request(evhtp_request_t* req) {
      Request r;

      switch (evhtp_request_get_method(req)) {
        case htp_method_GET:       r.method = "GET"; break;
        case htp_method_HEAD:      r.method = "HEAD"; break;
        case htp_method_POST:      r.method = "POST"; break;
        case htp_method_PUT:       r.method = "PUT"; break;
        case htp_method_DELETE:    r.method = "DELETE"; break;
        case htp_method_MKCOL:     r.method = "MKCOL"; break;
        case htp_method_COPY:      r.method = "COPY"; break;
        case htp_method_MOVE:      r.method = "MOVE"; break;
        case htp_method_OPTIONS:   r.method = "OPTIONS"; break;
        case htp_method_PROPFIND:  r.method = "PROPFIND"; break;
        case htp_method_PROPPATCH: r.method = "PROPPATCH"; break;
        case htp_method_LOCK:      r.method = "LOCK"; break;
        case htp_method_UNLOCK:    r.method = "UNLOCK"; break;
        case htp_method_TRACE:     r.method = "TRACE"; break;
        case htp_method_CONNECT:   r.method = "CONNECT"; break;
        case htp_method_PATCH:     r.method = "PATCH"; break;
        case htp_method_UNKNOWN:   r.method = "UNKNOWN"; break;
      }

      auto headers = req->headers_in;
      for (auto header = headers->tqh_first; header; header = header->next.tqe_next) {
        r.headers[std::string(header->key, header->klen)] = std::string(header->val, header->vlen);
      }

      auto host_it = r.headers.find("Host");
      std::string vhost;
      if (host_it != r.headers.end()) {
        vhost = host_it->second; // TODO: Split port from host
      }

      auto body = req->buffer_in;
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

      auto uri = req->uri;
      const char* scheme = "";
      switch (uri->scheme) {
        case htp_scheme_ftp:     scheme = "ftp"; break;
        case htp_scheme_http:    scheme = "http"; break;
        case htp_scheme_https:   scheme = "https"; break;
        case htp_scheme_nfs:     scheme = "nfs"; break;
        default: break;
      }

      r.params = data_franca::Object::dictionary();

      if (uri->query) {
        for (auto param = uri->query->tqh_first; param; param = param->next.tqe_next) {
          std::string k { param->key, param->klen };
          std::string v { param->val, param->vlen };
          k = URI::decode(k);
          v = URI::decode(v);
          add_destructured_param_to_dict(r.params, std::move(k), std::move(v));
        }
      }

      if (r.method == "POST" && r.body.size()) {
        printf("POST BODY: %s\n", r.body.c_str());
        auto kv_pairs = split(r.body, "&");
        for (auto& p: kv_pairs) {
          auto kv = split(p, "=", 2);
          if (kv.size() < 2) continue;
          auto k = URI::decode(kv[0]);
          auto v = URI::decode(kv[1]);
          add_destructured_param_to_dict(r.params, std::move(k), std::move(v));
        }
      }

      std::string query_raw { uri->query_raw ? reinterpret_cast<char*>(uri->query_raw) : "" };
      std::string fragment  { uri->fragment  ? reinterpret_cast<char*>(uri->fragment)  : "" };
      r.uri = URI{scheme, vhost, 80 /* TODO */, uri->path->full, std::move(query_raw), std::move(fragment)};
      return r;
    }

    Response make_response_from_evhttp_request(evhtp_request_t* req) {
      Response r;
      r.code = (HTTPStatusCode)req->status;

      auto headers = req->headers_in;
      for (auto header = headers->tqh_first; header; header = header->next.tqe_next) {
        std::string k { header->key, header->klen };
        std::string v { header->val, header->vlen };
        r.headers[std::move(k)] = std::move(v);
      }

      // XXX: Temporary workaround for a bug in libevhtp that causes non-error Status to be dropped from responses.
      if (r.headers.find("Location") != r.headers.end()) {
        r.code = HTTPStatusCode::Found;
      }
      // End temporary workaround.

      evbuffer* body = req->buffer_in;
      // TODO: Insert length sanity checks?
      size_t body_len = evbuffer_get_length(body);
      r.body.resize(body_len);
      evbuffer_copyout(body, &r.body[0], body_len);
      return std::move(r);
    }

    void add_evhttp_request_headers_and_body(evhtp_request_t* req, const Request& r) {
      auto headers = req->headers_out;
      for (auto& pair: r.headers) {
        auto kv = evhtp_header_new(pair.first.c_str(), pair.second.c_str(), 0, 0); // 0 means "do not copy"
        evhtp_headers_add_header(headers, kv);
      }

      if (r.body.size()) {
        evbuffer* body = req->buffer_out;
        evbuffer_add(body, r.body.data(), r.body.size());
      }
    }

    void send_response(const Response& response, evhtp_request_t* handle) {
      auto headers = handle->headers_out;
      for (auto& pair: response.headers) {
        auto kv = evhtp_header_new(pair.first.c_str(), pair.second.c_str(), 0, 0); // 0 means "do not copy"
        evhtp_headers_add_header(headers, kv);
      }

      evbuffer* body_buffer = handle->buffer_out;
      evbuffer_add(body_buffer, response.body.c_str(), response.body.size());
      evhtp_send_reply(handle, (int)response.code);
    }
  }

  struct HTTPServer::Private {
    evhtp_t* http = nullptr;
    std::function<Response(Request)> handler;

    std::mutex event_loops_lock;
    std::vector<std::unique_ptr<EventLoop>> event_loops;

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
    static void http_server_callback(evhtp_request_t* req, void* userdata) {
      auto p = static_cast<HTTPServer::Private*>(userdata);
      fiber::start([=]() {
        auto request = make_request_from_evhttp_request(req);
        auto response = p->handler(std::move(request));
        response.headers["Date"] = DateTime::now().strftime("%a, %d %b %y %T %z");
        send_response(response, req);
      });
    }


    static void http_server_init_thread(evhtp_t* htp, evthr_t* thr, void* userdata) {
      auto p = static_cast<HTTPServer::Private*>(userdata);
      auto base = evthr_get_base(thr);
      auto loop = new EventLoop { (void*)base };
      {
        std::unique_lock<std::mutex> L { p->event_loops_lock };
        p->event_loops.emplace_back(loop);
        std::string thread_name = wayward::format("Wayward HTTP Server Worker {0}", p->event_loops.size());
        #if defined(__linux__)
        ::pthread_setname_np(::pthread_self(), thread_name.c_str());
        #elif defined(__APPLE__)
        ::pthread_setname_np(thread_name.c_str());
        #endif
      }
      set_current_event_loop(loop);
    }
  }

  void HTTPServer::start(IEventLoop* loop) {
    assert(p_->http == nullptr);
    event_base* base = (event_base*)loop->native_handle();
    p_->http = evhtp_new(base, this);
    evhtp_set_gencb(p_->http, http_server_callback, p_.get());
    evhtp_use_threads(p_->http, http_server_init_thread, 8, p_.get());

    if (p_->socket_fd >= 0) {
      int r = evhtp_accept_socket(p_->http, p_->socket_fd, 5);
      if (r < 0) {
        throw HTTPError(wayward::format("Could not listen on provided socket: {0}.", p_->socket_fd));
      }
    } else {
      int r = evhtp_bind_socket(p_->http, p_->listen_host.c_str(), p_->port, 5);
      if (r < 0) {
        throw HTTPError(wayward::format("Could not bind to socket on {0}:{1}.", p_->listen_host, p_->port));
      }
    }
  }

  void HTTPServer::stop() {
    // XXX: Some way to check if requests are being served?
    std::unique_lock<std::mutex> L { p_->event_loops_lock };
    p_->event_loops.clear();
    evhtp_free(p_->http);
    p_->http = nullptr;
  }

  struct HTTPClient::Private {
    IEventLoop* loop = nullptr;
    evhtp_connection_t* conn = nullptr;
    std::string host;
    int port;

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
    p_->conn = evhtp_connection_new(base, host.c_str(), port);
    if (p_->conn == nullptr) {
      throw HTTPError(wayward::format("Could not connect to {0}:{1}.", host, port));
    }
    p_->host = std::move(host);
    p_->port = port;
  }

  HTTPClient::~HTTPClient() {}

  std::string HTTPClient::host() const {
    return p_->host;
  }

  int HTTPClient::port() const {
    return p_->port;
  }

  namespace {
    static void http_client_callback(evhtp_request_t* req, void* userdata) {
      HTTPClient* client = static_cast<HTTPClient*>(userdata);
      client->p_->recorded_response = make_response_from_evhttp_request(req);
      fiber::resume(std::move(client->p_->initiating_fiber));
    }
  }

  Response HTTPClient::request(Request req) {
    assert(p_->conn);
    evhtp_request_t* r = evhtp_request_new(http_client_callback, this);
    htp_method method;
    if (req.method == "GET")       { method = htp_method_GET; } else
    if (req.method == "HEAD")      { method = htp_method_HEAD; } else
    if (req.method == "POST")      { method = htp_method_POST; } else
    if (req.method == "PUT")       { method = htp_method_PUT; } else
    if (req.method == "DELETE")    { method = htp_method_DELETE; } else
    if (req.method == "MKCOL")     { method = htp_method_MKCOL; } else
    if (req.method == "COPY")      { method = htp_method_COPY; } else
    if (req.method == "MOVE")      { method = htp_method_MOVE; } else
    if (req.method == "OPTIONS")   { method = htp_method_OPTIONS; } else
    if (req.method == "PROPFIND")  { method = htp_method_PROPFIND; } else
    if (req.method == "PROPPATCH") { method = htp_method_PROPPATCH; } else
    if (req.method == "LOCK")      { method = htp_method_LOCK; } else
    if (req.method == "UNLOCK")    { method = htp_method_UNLOCK; } else
    if (req.method == "TRACE")     { method = htp_method_TRACE; } else
    if (req.method == "CONNECT")   { method = htp_method_CONNECT; } else
    if (req.method == "PATCH")     { method = htp_method_PATCH; } else
    if (req.method == "UNKNOWN")   { method = htp_method_UNKNOWN; } else
    {
      throw HTTPError(wayward::format("Invalid method: {0}", req.method));
    }

    for (auto& pair: req.headers) {
      auto kv = evhtp_header_new(pair.first.c_str(), pair.second.c_str(), 0, 0); // 0 means "do not copy"
      evhtp_headers_add_header(r->headers_out, kv);
    }

    if (req.body.size()) {
      evbuffer_add(r->buffer_out, req.body.c_str(), req.body.size());
      evbuffer_add(r->buffer_out, "\n\n", 2);
    }

    p_->initiating_fiber = fiber::current();
    p_->recorded_response = Nothing;
    p_->error = nullptr;
    int result = evhtp_make_request(p_->conn, r, method, req.uri.path.c_str());
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
    req.uri.path = std::move(path);
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

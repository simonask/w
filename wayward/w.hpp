#pragma once
#ifndef W_HPP_INCLUDED
#define W_HPP_INCLUDED

#include <string>
#include <functional>
#include <vector>
#include <memory>
#include <map>

#include <wayward/http.hpp>
#include <wayward/support/uri.hpp>
#include <wayward/support/format.hpp>
#include <wayward/support/logger.hpp>
#include <wayward/support/node.hpp>

#if !defined(WAYWARD_NO_SHORTHAND_NAMESPACE)
namespace w = wayward;
#endif

namespace wayward {
  struct Request {
    std::map<std::string, std::string> headers;
    std::map<std::string, Node> params;
    std::string method;
    URI uri;
    std::string body;
  };

  struct Response {
    std::map<std::string, std::string> headers;
    HTTPStatusCode code = HTTPStatusCode::OK;
    std::string reason; // Keep empty to derive from `code`.
    std::string body;
  };

  template <typename... Args>
  Response render_text(std::string text, Args&&... args) {
    Response r;
    r.code = HTTPStatusCode::OK;
    r.headers["Content-Type"] = "text/plain";
    r.body = wayward::format(text, std::forward<Args>(args)...);
    return r;
  }

  inline Response not_found() {
    Response r;
    r.code = HTTPStatusCode::NotFound;
    r.body = "Not Found";
    r.headers["Content-Type"] = "text/plain";
    return r;
  }

  Response render_template(std::string templ);
  Response redirect(std::string new_location, HTTPStatusCode code = HTTPStatusCode::Found);

  struct IScope {
    virtual ~IScope() {}
    virtual void get(std::string path, std::function<Response(Request&)> handler) = 0;
    virtual void put(std::string path, std::function<Response(Request&)> handler) = 0;
    virtual void post(std::string path, std::function<Response(Request&)> handler) = 0;
    virtual void del(std::string path, std::function<Response(Request&)> handler) = 0;
    virtual void head(std::string path, std::function<Response(Request&)> handler) = 0;
    virtual void options(std::string path, std::function<Response(Request&)> handler) = 0;
  };

  class App : public IScope {
  public:
    App();
    virtual ~App();

    struct {
      bool log_requests = true;
      bool parallel = false;
    } config;

    int listen_and_serve(std::string address = "0.0.0.0", int port = 3000);

    void get(std::string path, std::function<Response(Request&)> handler) override;
    void put(std::string path, std::function<Response(Request&)> handler) override;
    void post(std::string path, std::function<Response(Request&)> handler) override;
    void del(std::string path, std::function<Response(Request&)> handler) override;
    void head(std::string path, std::function<Response(Request&)> handler) override;
    void options(std::string path, std::function<Response(Request&)> handler) override;

    void print_routes() const;

    struct Private;
    std::unique_ptr<Private> priv;
  };

  std::shared_ptr<ILogger> logger();
  void set_logger(std::shared_ptr<ILogger>);

  namespace log {
    void debug(std::string tag, std::string message);
    void debug(std::string message);
    void info(std::string tag, std::string message);
    void info(std::string message);
    void warning(std::string tag, std::string message);
    void warning(std::string message);
    void error(std::string tag, std::string message);
    void error(std::string message);
  }
}

#endif /* end of include guard: SYMBOL */

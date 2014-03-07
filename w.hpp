#pragma once
#ifndef W_HPP_INCLUDED
#define W_HPP_INCLUDED

#include <string>
#include <functional>
#include <vector>
#include <memory>
#include <map>
#include "uri.hpp"

namespace w {
  struct Request {
    std::map<std::string, std::string> headers;
    std::string method;
    URI uri;
    std::string body;
  };

  struct Response {
    std::map<std::string, std::string> headers;
    int code = 200;
    std::string reason; // "OK", "Not Found" etc.
    std::string body; // TODO: Use a Rope?
  };

  Response render_text(std::string text);

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
    } config;

    int listen_and_serve(std::string address = "0.0.0.0", int port = 3000);

    void get(std::string path, std::function<Response(Request&)> handler) override;
    void put(std::string path, std::function<Response(Request&)> handler) override;
    void post(std::string path, std::function<Response(Request&)> handler) override;
    void del(std::string path, std::function<Response(Request&)> handler) override;
    void head(std::string path, std::function<Response(Request&)> handler) override;
    void options(std::string path, std::function<Response(Request&)> handler) override;

    struct Private;
    std::unique_ptr<Private> priv;
  };
}

#endif /* end of include guard: SYMBOL */

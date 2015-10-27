#pragma once
#ifndef W_HPP_INCLUDED
#define W_HPP_INCLUDED

#include <string>
#include <functional>
#include <vector>
#include <memory>
#include <map>

#include <wayward/support/http.hpp>
#include <wayward/support/uri.hpp>
#include <wayward/support/format.hpp>
#include <wayward/support/logger.hpp>
#include <wayward/support/json.hpp>
#include <wayward/support/plugin.hpp>

#include <wayward/template_engine.hpp>
#include <wayward/session.hpp>
#include <wayward/respond_to.hpp>

#if !defined(WAYWARD_NO_SHORTHAND_NAMESPACE)
namespace w = wayward;
#endif

namespace wayward {
  template <typename... Args>
  Response render_text(std::string text, Args&&... args) {
    Response r;
    r.code = HTTPStatusCode::OK;
    r.headers["Content-Type"] = "text/plain; charset=utf-8";
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

  Response render(const std::string& template_name, Options params = Options{}, HTTPStatusCode code = HTTPStatusCode::OK);
  Response redirect(std::string new_location, HTTPStatusCode code = HTTPStatusCode::Found);
  Response file(std::string path, Maybe<std::string> content_type = Nothing);

  struct Scope {
    virtual ~Scope() {}
    void get(std::string path, std::function<Response(Request&)> handler)     { add_route("GET",     std::move(path), std::move(handler)); }
    void put(std::string path, std::function<Response(Request&)> handler)     { add_route("PUT",     std::move(path), std::move(handler)); }
    void patch(std::string path, std::function<Response(Request&)> handler)   { add_route("PATCH",   std::move(path), std::move(handler)); }
    void post(std::string path, std::function<Response(Request&)> handler)    { add_route("POST",    std::move(path), std::move(handler)); }
    void del(std::string path, std::function<Response(Request&)> handler)     { add_route("DELETE",  std::move(path), std::move(handler)); }
    void head(std::string path, std::function<Response(Request&)> handler)    { add_route("HEAD",    std::move(path), std::move(handler)); }
    void options(std::string path, std::function<Response(Request&)> handler) { add_route("OPTIONS", std::move(path), std::move(handler)); }

    template <typename R> using RouteMethodPointer = Response(R::*)(Request&);
    template <typename R> void     get(std::string path, RouteMethodPointer<R> handler) {     get(std::move(path), make_handler_for_route_method(handler)); }
    template <typename R> void     put(std::string path, RouteMethodPointer<R> handler) {     put(std::move(path), make_handler_for_route_method(handler)); }
    template <typename R> void   patch(std::string path, RouteMethodPointer<R> handler) {   patch(std::move(path), make_handler_for_route_method(handler)); }
    template <typename R> void    post(std::string path, RouteMethodPointer<R> handler) {    post(std::move(path), make_handler_for_route_method(handler)); }
    template <typename R> void     del(std::string path, RouteMethodPointer<R> handler) {     del(std::move(path), make_handler_for_route_method(handler)); }
    template <typename R> void    head(std::string path, RouteMethodPointer<R> handler) {    head(std::move(path), make_handler_for_route_method(handler)); }
    template <typename R> void options(std::string path, RouteMethodPointer<R> handler) { options(std::move(path), make_handler_for_route_method(handler)); }

    virtual void add_route(std::string method, std::string path, std::function<Response(Request&)> handler) = 0;

  private:
    template <typename R>
    std::function<Response(Request&)>
    make_handler_for_route_method(RouteMethodPointer<R> handler) {
      return [=](Request& req) -> Response {
        R routes;
        routes.before(req);
        auto response = routes.around(req, [&](Request& r) { return (routes.*handler)(r); });
        // TODO: Make the response available to "after" filters.
        routes.after(req);
        return std::move(response);
      };
    }
  };

  class App : public Scope {
  public:
    App(int argc, char const* const* argv);
    virtual ~App();

    struct {
      bool log_requests = true;
      bool parallel = false;
    } config;

    std::string root() const;
    int run();
    void add_route(std::string method, std::string path, std::function<Response(Request&)> handler) final;
    void assets(std::string uri_path, std::string filesystem_path);
    void print_routes() const;
    Response request(Request);

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

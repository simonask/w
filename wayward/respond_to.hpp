#pragma once
#ifndef WAYWARD_RESPOND_TO_HPP_INCLUDED
#define WAYWARD_RESPOND_TO_HPP_INCLUDED

#include <wayward/content_type.hpp>
#include <wayward/support/http.hpp>

#include <map>

namespace wayward {
  template <typename T>
  Response render_json(T&& object) {
    Response response;
    response.headers["Content-Type"] = "application/json; charset=utf-8";
    response.body = as_json(std::forward<T>(object));
    return std::move(response);
  }

  struct Responder {
    explicit Responder(Request&);
    explicit Responder(Request&&);
    Responder(Request&, std::string default_content_type);
    Responder(Request&&, std::string default_content_type);

    operator Response() const;
    Response to_response() const;

    template <typename ContentType, typename F>
    Responder& when(F&& f);

    template <typename F>
    Responder& when(std::string content_type, F&& f);

    template <typename T>
    Responder& with(T&& object);

  private:
    Maybe<std::string> default_content_type_;
    Maybe<Request> owned_request_;
    Request& request_;
    std::map<std::string, std::function<Response()>> responders_;
  };

  template <typename ContentType, typename F>
  Responder& Responder::when(F&& f) {
    return when(ContentType::MimeType, std::forward<F>(f));
  }

  template <typename F>
  Responder& Responder::when(std::string content_type, F&& f) {
    responders_[content_type] = std::function<Response()>{ std::forward<F>(f) };
    return *this;
  }

  inline Responder::operator Response() const {
    return to_response();
  }

  inline Responder respond_to(Request& req) {
    return Responder{req};
  }

  template <typename DefaultContentType>
  inline Responder respond_to(Request& req) {
    return Responder{req, DefaultContentType::MimeType};
  }
}

#endif // WAYWARD_RESPOND_TO_HPP_INCLUDED

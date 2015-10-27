#pragma once
#ifndef WAYWARD_RESPOND_TO_HPP_INCLUDED
#define WAYWARD_RESPOND_TO_HPP_INCLUDED

#include <wayward/content_type.hpp>
#include <wayward/support/http.hpp>
#include <wayward/content_type.hpp>

#include <set>

namespace wayward {
  template <typename T>
  Response render_json(T&& object) {
    Response response;
    response.headers["Content-Type"] = "application/json; charset=utf-8";
    response.body = as_json(std::forward<T>(object));
    return std::move(response);
  }

  struct Responder {
    explicit Responder(const Request&);
    Responder(const Request&, std::string default_content_type);

    operator Response() const;
    Response to_response() const&;
    Response to_response() &&;

    template <typename ContentType, typename F>
    Responder& when(F&& f);

    template <typename F>
    Responder& when(std::string format, F&& f);

    template <typename F>
    Responder& when_accepting(std::string content_type, F&& f);

    template <typename F>
    Responder& otherwise(F&& f);

    template <typename T>
    Responder& with(T&& object);

  private:
    std::set<std::string> accepted_content_types_;
    Maybe<std::string> default_content_type_;
    Maybe<Response> generated_response_;
    const Request& request_;

    bool should_accept_content_type(const std::string& content_type) const;
  };

  template <typename ContentType, typename F>
  Responder& Responder::when(F&& f) {
    return when_accepting(ContentType::MimeType, std::forward<F>(f));
  }

  template <typename F>
  Responder& Responder::when(std::string ext, F&& f) {
    return when_accepting(*content_type_for_extension(ext), std::forward<F>(f));
  }

  template <typename F>
  Responder& Responder::when_accepting(std::string content_type, F&& f) {
    if (should_accept_content_type(content_type)) {
      generated_response_ = f();
    }
    return *this;
  }

  template <typename F>
  Responder& Responder::otherwise(F&& f) {
    if (!generated_response_) {
      generated_response_ = f();
    }
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

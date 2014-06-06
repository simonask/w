#pragma once
#ifndef WAYWARD_SUPPORT_RESULT_HPP_INCLUDED
#define WAYWARD_SUPPORT_RESULT_HPP_INCLUDED

#include <wayward/support/error.hpp>
#include <wayward/support/either.hpp>
#include <wayward/support/monad.hpp>

namespace wayward {
  using ErrorPtr = std::unique_ptr<Error>;

  template <typename T>
  struct Result {
    Result(T result) : result_{std::move(result)} {}
    Result(ErrorPtr error) : result_{std::move(error)} {}

    bool is_error() const { return !result_.template is_a<T>(); }
    bool good() const { return !is_error(); }

    T get_result() && { return std::move(result_.template get<T>()); }
    ErrorPtr get_error_ptr() && { return std::move(result_.template get<ErrorPtr>()); }
  private:
    Either<T, ErrorPtr> result_;
  };

  namespace monad {
    template <typename T>
    struct Join<Result<Result<T>>> {
      using Type = Result<T>;
    };
    template <typename T>
    struct Join<Result<T>> {
      using Type = Result<T>;
    };

    template <typename T>
    struct Bind<Result<T>> {
      template <typename F>
      auto bind(Result<T> result, F f) -> typename Join<decltype(f(std::declval<T>()))>::Type {
        if (result.is_error()) {
          return std::move(result).get_error_ptr();
        } else {
          return f(std::move(result).get_result());
        }
      }
    };
  }
}

#endif // WAYWARD_SUPPORT_RESULT_HPP_INCLUDED

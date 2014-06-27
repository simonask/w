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
    Result(Result<T>&&) = default;
    Result(const Result<T>&) = delete;
    Result& operator=(Result<T>&&) = default;
    Result& operator=(const Result<T>&) = delete;

    bool is_error() const { return !result_.template is_a<T>(); }
    bool good() const { return !is_error(); }
    explicit operator bool() const { return good(); }

    T& get() & { return *result_.template get<T&>(); }
    const T& get() const& { return *result_.template get<const T&>(); }
    T get() && { return std::move(*std::move(result_).template get<T>()); }

    ErrorPtr& error() & { return *result_.template get<ErrorPtr&>(); }
    const ErrorPtr& error() const& { return *result_.template get<const ErrorPtr&>(); }
    ErrorPtr error() && { return std::move(*std::move(result_).template get<ErrorPtr>()); }
  private:
    Either<T, ErrorPtr> result_;
  };

  struct SuccessType { constexpr SuccessType() {} };
  static const constexpr SuccessType Success;

  template <>
  struct Result<void> {
    Result() {}
    Result(NothingType) {}
    Result(SuccessType) {}
    Result(ErrorPtr error) : error_(std::move(error)) {}

    bool is_error() const { return (bool)error_; }
    bool good() const { return !is_error(); }
    explicit operator bool() const { return good(); }

    ErrorPtr& error() & { return error_; }
    const ErrorPtr& error() const& { return error_; }
    ErrorPtr error() && { return std::move(error_); }
  private:
    ErrorPtr error_;
  };

  template <typename E, typename... Args>
  ErrorPtr make_error(Args&&... args) {
    return ErrorPtr{ new E{std::forward<Args>(args)...} };
  }

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
      template <typename R, typename F>
      static auto bind(R&& result, F&& f) -> typename Join<Result<decltype(f(std::declval<T>()))>>::Type {
        if (result.good()) {
          return f(std::move((std::forward<R>(result).get())));
        } else {
          return std::move((std::forward<R>(result).error()));
        }
      }
    };

    template <>
    struct Bind<Result<void>> {
      template <typename R, typename F>
      static auto bind(R&& result, F&& f) -> typename Join<Result<decltype(f())>>::Type {
        if (result.good()) {
          return f();
        } else {
          return std::forward<R>(result).error();
        }
      }
    };
  }
}

#endif // WAYWARD_SUPPORT_RESULT_HPP_INCLUDED

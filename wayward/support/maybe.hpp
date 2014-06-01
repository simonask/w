#pragma once
#ifndef WAYWARD_SUPPORT_MAYBE_HPP_INCLUDED
#define WAYWARD_SUPPORT_MAYBE_HPP_INCLUDED

#include <type_traits>

#include <wayward/support/error.hpp>
#include <wayward/support/monad.hpp>

namespace wayward {
  struct EmptyMaybeDereference : Error {
    EmptyMaybeDereference() : Error("Attempt to dereference a Maybe object that was empty.") {}
  };

  struct NothingType { constexpr NothingType() {} };
  static const constexpr NothingType Nothing;

  /*
    NOTE: This is equivalent to std::optional/boost::optional, but we must not introduce dependencies!
  */
  template <typename T>
  class Maybe {
  public:
    constexpr Maybe() {}
    Maybe(T value) : has_value_(true) { new(&storage_) T(std::move(value)); }
    Maybe(const Maybe<T>& other);
    Maybe(Maybe<T>&& other) : has_value_(false) { swap(other); }
    constexpr Maybe(NothingType) : has_value_(false) {}
    ~Maybe();
    Maybe<T>& operator=(T value);
    Maybe<T>& operator=(const Maybe<T>& value);
    Maybe<T>& operator=(Maybe<T>&& value);

    T* get() { if (has_value_) return reinterpret_cast<T*>(&storage_); throw EmptyMaybeDereference(); }
    const T* get() const { if (has_value_) return reinterpret_cast<const T*>(&storage_); throw EmptyMaybeDereference(); }

    bool operator==(NothingType) const { return !has_value_; }
    bool operator!=(NothingType) const { return has_value_; }

    explicit operator bool() const { return has_value_; }
    const T* operator->() const { return get(); }
    const T& operator*() const { return *get(); }
    T* operator->() { return get(); }
    T& operator*() { return *get(); }

    void swap(Maybe<T>& other);
  private:
    typename std::aligned_storage<sizeof(T), alignof(T)>::type storage_;
    bool has_value_ = false;
  };

  template <typename T, typename F>
  void when_maybe(const Maybe<T>& m, F then) {
    if (m) {
      then(*m);
    }
  }

  template <typename T, typename F>
  void when_maybe(Maybe<T>& m, F then) {
    if (m) {
      then(*m);
    }
  }

  template <typename T>
  Maybe<T>::Maybe(const Maybe<T>& other) : has_value_(other.has_value_) {
    if (has_value_) {
      new(get()) T(*other);
    }
  }

  template <typename T>
  Maybe<T>::~Maybe() {
    if (has_value_) get()->~T();
  }

  template <typename T>
  Maybe<T>& Maybe<T>::operator=(T value) {
    if (has_value_) {
      *get() = std::move(value);
    } else {
	  has_value_ = true;
      new(&storage_) T(std::move(value));
    }
    return *this;
  }

  template <typename T>
  Maybe<T>& Maybe<T>::operator=(Maybe<T>&& value) {
    swap(value);
    return *this;
  }

  template <typename T>
  Maybe<T>& Maybe<T>::operator=(const Maybe<T>& value) {
    Maybe<T> copy = value;
    swap(copy);
    return *this;
  }

  template <typename T>
  void Maybe<T>::swap(Maybe<T>& other) {
    if (has_value_) {
      if (other.has_value_) {
        std::swap(*get(), *other.get());
      } else {
        new(&other.storage_) T(std::move(*get()));
        get()->~T();
        std::swap(has_value_, other.has_value_);
      }
    } else {
      if (other.has_value_) {
        new(&storage_) T(std::move(*other.get()));
        other.get()->~T();
        std::swap(has_value_, other.has_value_);
      } else {
        // Neither has a value, so do nothing.
      }
    }
  }

  template <typename T>
  Maybe<T> Just(T&& x) {
    return Maybe<T>(T(std::forward<T>(x)));
  }

  namespace monad {
    template <typename T> struct Join<Maybe<Maybe<T>>> {
      using Type = Maybe<T>;
    };
    template <typename T> struct Join<Maybe<T>> {
      using Type = Maybe<T>;
    };

    template <typename T>
    struct Bind<Maybe<T>> {
      template <typename F>
      static auto bind(Maybe<T> m, F f) -> typename Join<Maybe<decltype(f(*m))>>::Type {
        if (m) {
          return f(*m);
        } else {
          return Nothing;
        }
      }
    };
  }
}

namespace std {
  template <typename T>
  void swap(wayward::Maybe<T>& a, wayward::Maybe<T>& b) {
    a.swap(b);
  }
}

#endif // WAYWARD_SUPPORT_MAYBE_HPP_INCLUDED

#pragma once
#ifndef WAYWARD_SUPPORT_MAYBE_HPP_INCLUDED
#define WAYWARD_SUPPORT_MAYBE_HPP_INCLUDED

#include <type_traits>
#include <stdexcept>

namespace wayward {
  struct EmptyMaybeDereference : std::runtime_error {
    EmptyMaybeDereference() : std::runtime_error("Attempt to dereference a Maybe object that was empty.") {}
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
    constexpr Maybe(NothingType) {}
    ~Maybe();
    Maybe<T>& operator=(T value);
    Maybe<T>& operator=(const Maybe<T>& value);
    Maybe<T>& operator=(Maybe<T>&& value);

    T* get() { if (has_value_) return reinterpret_cast<T*>(&storage_); throw EmptyMaybeDereference(); }
    const T* get() const { if (has_value_) return reinterpret_cast<const T*>(&storage_); throw EmptyMaybeDereference(); }

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
        std::swap(has_value_, other.has_value_);
        get()->~T();
      }
    } else {
      if (other.has_value_) {
        new(&storage_) T(std::move(*other.get()));
        std::swap(has_value_, other.has_value_);
        other.get()->~T();
      } else {
        // Neither has a value, so do nothing.
      }
    }
  }
}

namespace std {
  template <typename T>
  void swap(wayward::Maybe<T>& a, wayward::Maybe<T>& b) {
    a.swap(b);
  }
}

#endif // WAYWARD_SUPPORT_MAYBE_HPP_INCLUDED

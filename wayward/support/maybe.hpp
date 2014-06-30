#pragma once
#ifndef WAYWARD_SUPPORT_MAYBE_HPP_INCLUDED
#define WAYWARD_SUPPORT_MAYBE_HPP_INCLUDED

#include <type_traits>

#include <wayward/support/error.hpp>
#include <wayward/support/monad.hpp>
#include <wayward/support/meta.hpp>

namespace wayward {
  struct EmptyMaybeDereference : Error {
    EmptyMaybeDereference() : Error("Attempt to dereference a Maybe object that was empty.") {}
  };

  struct NothingType { constexpr NothingType() {} };
  static const constexpr NothingType Nothing;

  /*
    NOTE: This is equivalent to std::optional/boost::optional, but we must not introduce dependencies!
  */
  template <typename T, typename Enable = void>
  class Maybe {
  public:
    constexpr Maybe() {}
    Maybe(T value) : has_value_(true) { new(&storage_) T(std::move(value)); }
    Maybe(const Maybe<T>& other);
    Maybe(Maybe<T>&& other) : has_value_(other.has_value_) { if (has_value_) new(unsafe_get()) T(std::move(*other.unsafe_get())); }
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

    T* unsafe_get() { return reinterpret_cast<T*>(&storage_); }
    const T* unsafe_get() const { return reinterpret_cast<const T*>(&storage_); }
  private:
    typename std::aligned_storage<sizeof(T), alignof(T)>::type storage_;
    bool has_value_ = false;
  };

  template <typename T>
  class Maybe<T&, void> {
  public:
    constexpr Maybe() : ref_(std::ref(*reinterpret_cast<T*>((void*)0))) {}
    Maybe(T& value) : ref_(std::ref(value)), has_value_(true) {}
    Maybe(const Maybe<T&>&) = default;
    constexpr Maybe(NothingType) : ref_(std::ref(*reinterpret_cast<T*>((void*)0))), has_value_(false) {}
    ~Maybe() {}
    Maybe<T&>& operator=(std::reference_wrapper<T> ref) { ref_ = ref; has_value_ = true; return *this; }
    Maybe<T&>& operator=(const Maybe<T&>& value) = default;

    T* get() { if (has_value_) return &ref_.get(); throw EmptyMaybeDereference(); }
    const T* get() const { if (has_value_) return &ref_.get(); throw EmptyMaybeDereference(); }

    bool operator==(NothingType) const { return !has_value_; }
    bool operator!=(NothingType) const { return has_value_; }

    explicit operator bool() const { return has_value_; }
    const T* operator->() const { return get(); }
    const T& operator*() const { return ref_; }
    T* operator->() { return get(); }
    T& operator*() { return ref_; }

    void swap(Maybe<T&>& other) { std::swap(ref_, other.ref_); std::swap(has_value_, other.has_value_); }

    T* unsafe_get() { return &ref_.get(); }
    const T* unsafe_get() const { return &ref_.get(); }
  private:
    std::reference_wrapper<T> ref_;
    bool has_value_ = false;
  };

  /*
    Specialize for pointer-like types — in case of a nullable pointer, we treat null as
    Nothing.
  */
  template <typename T>
  class Maybe<T, typename std::enable_if<meta::IsPointerLike<T>::Value>::type> {
  public:
    constexpr Maybe() : ptr_{nullptr} {}
    Maybe(T value) : ptr_(std::move(value)) {}
    Maybe(Maybe<T>&& other) = default;
    Maybe(const Maybe<T>& other) = default;
    constexpr Maybe(NothingType) : ptr_{nullptr} {}
    ~Maybe() {}
    Maybe<T>& operator=(const Maybe<T>& value) = default;
    Maybe<T>& operator=(Maybe<T>&& value) = default;

    T* get() { if (ptr_) return &ptr_; throw EmptyMaybeDereference(); }
    const T* get() const { if (ptr_) return &ptr_; throw EmptyMaybeDereference(); }

    bool operator==(NothingType) const { return ptr_ == nullptr; }
    bool operator!=(NothingType) const { return ptr_ != nullptr; }

    explicit operator bool() const { return (bool)ptr_; }
    const T* operator->() const { return get(); }
    const T& operator*() const { return ptr_; }
    T* operator->() { return get(); }
    T& operator*() { return ptr_; }

    void swap(Maybe<T>& other) { std::swap(ptr_, other.ptr_); }

    T* unsafe_get() { return &ptr_; }
    const T* unsafe_get() const { return &ptr_; }
  private:
    T ptr_;
  };

  template <typename T, typename F>
  void when_maybe(const Maybe<T>& m, F&& then) {
    if (m) {
      then(*m);
    }
  }

  template <typename T, typename F>
  void when_maybe(Maybe<T>& m, F&& then) {
    if (m) {
      then(*m);
    }
  }

  template <typename T, typename Enable>
  Maybe<T, Enable>::Maybe(const Maybe<T>& other) : has_value_(other.has_value_) {
    if (has_value_) {
      new(get()) T(*other);
    }
  }

  template <typename T, typename Enable>
  Maybe<T, Enable>::~Maybe() {
    if (has_value_) get()->~T();
  }

  template <typename T, typename Enable>
  Maybe<T>& Maybe<T, Enable>::operator=(T value) {
    if (has_value_) {
      *get() = std::move(value);
    } else {
	  has_value_ = true;
      new(&storage_) T(std::move(value));
    }
    return *this;
  }

  template <typename T, typename Enable>
  Maybe<T>& Maybe<T, Enable>::operator=(Maybe<T>&& value) {
    swap(value);
    return *this;
  }

  template <typename T, typename Enable>
  Maybe<T>& Maybe<T, Enable>::operator=(const Maybe<T>& value) {
    Maybe<T> copy = value;
    swap(copy);
    return *this;
  }

  template <typename T, typename Enable>
  void Maybe<T, Enable>::swap(Maybe<T>& other) {
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
    template <> struct Join<Maybe<void>> {
      using Type = void;
    };

    template <typename T, typename RT> struct BindMaybeReturnWrapper;
    template <typename T> struct BindMaybeReturnWrapper<T, void> {
      template <typename F>
      static auto wrap(Maybe<T> m, F&& f) -> void {
        if (m) {
          f(*m);
        }
      }
    };
    template <typename T, typename RT> struct BindMaybeReturnWrapper {
      template <typename F>
      static auto wrap(Maybe<T> m, F&& f) -> RT {
        if (m) {
          return f(*m);
        }
        return Nothing;
      }
    };

    template <typename T>
    struct Bind<Maybe<T>> {
      template <typename F>
      static auto bind(Maybe<T> m, F&& f) -> typename Join<Maybe<decltype(f(*m))>>::Type {
        using RT = typename Join<Maybe<decltype(f(*m))>>::Type;
        return BindMaybeReturnWrapper<T, RT>::wrap(m, std::forward<F>(f));
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

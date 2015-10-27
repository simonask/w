#pragma once
#ifndef WAYWARD_SUPPORT_CLONING_PTR_HPP_INCLUDED
#define WAYWARD_SUPPORT_CLONING_PTR_HPP_INCLUDED

#include <memory>
#include <wayward/support/meta.hpp>

namespace wayward {
  struct ICloneable {
    virtual ICloneable* clone() const = 0;
  };

  template <typename T, typename Base = ICloneable>
  struct Cloneable : Base {
    using Self_ = Cloneable<T, Base>;

    template <typename... Args>
    Cloneable(Args&&... args) : Base(std::forward<Args>(args)...) {}
    Cloneable(const Self_&) = default;
    Cloneable(Self_&&) = default;

    Base* clone() const { return new T(dynamic_cast<const T&>(*this)); }
  };

  /*
    CloningPtr is exactly the same as a std::unique_ptr, except
    it has a copy-constructor, which clones the heap object.

    CloningPtr does not support custom deleters/allocators at this moment.
  */
  template <typename T>
  struct CloningPtr {
    using pointer = typename std::unique_ptr<T>::pointer;
    using element_type = typename std::unique_ptr<T>::element_type;

    CloningPtr() {}

    explicit CloningPtr(pointer obj) : ptr(obj) {}

    CloningPtr(std::nullptr_t) : ptr(nullptr) {}

    template <typename U>
    CloningPtr(const CloningPtr<U>& other) : ptr(other.ptr ? dynamic_cast<U*>(other.ptr->clone()) : nullptr) {}

    CloningPtr(const CloningPtr<T>& other) : ptr(other.ptr ? dynamic_cast<T*>(other.ptr->clone()) : nullptr) {}

    template <typename U>
    CloningPtr(CloningPtr<U>&& other) : ptr(std::move(other.unique_ptr())) {}

    template <typename U>
    CloningPtr<T>& operator=(const CloningPtr<U>& other) {
      ptr = other.ptr ? std::unique_ptr<U>(dynamic_cast<U*>(other.ptr->clone())) : nullptr;
      return *this;
    }

    CloningPtr<T>& operator=(const CloningPtr<T>& other) {
      ptr = other.ptr ? std::unique_ptr<T>(dynamic_cast<T*>(other.ptr->clone())) : nullptr;
      return *this;
    }

    template <typename U>
    CloningPtr<T>& operator=(CloningPtr<U>&& other) {
      ptr = std::move(other.unique_ptr());
      return *this;
    }

    std::unique_ptr<T>& unique_ptr() { return ptr; }
    const std::unique_ptr<T>& unique_ptr() const { return ptr; }

    pointer release() { return ptr.release(); }
    void reset(pointer ptr = pointer()) { ptr.reset(ptr); }
    void swap(CloningPtr<T>& other) { ptr.swap(other.ptr); }
    pointer get() { return ptr.get(); }
    const pointer get() const { return ptr.get(); }
    explicit operator bool() const { return ptr.operator bool(); }
    T& operator*() const { return *ptr; }
    pointer operator->() const { return ptr.operator->(); }
  private:
    std::unique_ptr<T> ptr;
  };

  template <typename T, typename U>
  bool operator==(const CloningPtr<T>& lhs, const CloningPtr<U>& rhs) {
    return lhs.get() == rhs.get();
  }

  template <typename T, typename U>
  bool operator!=(const CloningPtr<T>& lhs, const CloningPtr<U>& rhs) {
    return lhs.get() != rhs.get();
  }

  template <typename T>
  bool operator==(const CloningPtr<T>& lhs, std::nullptr_t) {
    return lhs.get() == nullptr;
  }

  template <typename T>
  bool operator==(std::nullptr_t, const CloningPtr<T>& rhs) {
    return rhs.get() == nullptr;
  }

  template <typename T>
  CloningPtr<T> make_cloning_ptr(T* ptr) {
    return CloningPtr<T>(ptr);
  }

  namespace meta {
    template <class T> struct IsPointerLike<CloningPtr<T>> : TrueType {};
  }
}

namespace std {
  template <typename T>
  void swap(wayward::CloningPtr<T>& a, wayward::CloningPtr<T>& b) {
    a.swap(b);
  }

  template <typename T>
  struct hash<wayward::CloningPtr<T>> {
    size_t operator()(const wayward::CloningPtr<T>& cptr) const {
      return std::hash<std::unique_ptr<T>>()(cptr.unique_ptr());
    }
  };
}

#endif // WAYWARD_SUPPORT_CLONING_PTR_HPP_INCLUDED

#pragma once
#ifndef WAYWARD_SUPPORT_ANY_HPP_INCLUDED
#define WAYWARD_SUPPORT_ANY_HPP_INCLUDED

#include <wayward/support/type_info.hpp>
#include <wayward/support/maybe.hpp>
#include <wayward/support/meta.hpp>

namespace wayward {
  struct Any {
    template <class T>
    Any(T&& object);
    template <class T>
    Any(const T& object);

    Any() {}
    Any(NothingType) {}

    Any(const Any& other);
    Any(Any&& other);
    ~Any() { destruct(); }
    Any& operator=(const Any&);
    Any& operator=(Any&&);

    const TypeInfo& type_info() const { return *type_info_; }

    template <class T> bool is_a() const;
    template <class T> Maybe<T> get();
    template <class T> Maybe<T> get() const;
    template <class T, class F> auto when(F&& f) -> typename monad::Join<decltype(f(std::declval<T>()))>::Type;
    template <class T, class F> auto when(F&& f) const -> typename monad::Join<decltype(f(std::declval<T>()))>::Type;

  private:
    const TypeInfo* type_info_ = &GetTypeInfo<NothingType>::Value;
    static const size_t SmallObjectStorageSize = sizeof(void*) * 3;
    static const size_t SmallObjectStorageAlignment = sizeof(void*);
    using SmallObjectStorage = std::aligned_storage<SmallObjectStorageSize, SmallObjectStorageAlignment>::type;

    union {
      SmallObjectStorage inline_storage_;
      void* heap_storage_ = nullptr;
    };

    bool is_small_object() const;
    void ensure_allocation();
    void destruct();
    void* memory();
    const void* memory() const;

    friend struct AnyRef;
    friend struct AnyConstRef;
  };

  struct AnyConstRef;

  struct AnyRef {
    template <class T>
    AnyRef(T& object,
      typename std::enable_if<!std::is_same<AnyRef, T>::value>::type* dummy = nullptr
      ) : type_info_(&GetTypeInfo<T>::Value), ref_(reinterpret_cast<void*>(&object)) {
      static_assert(!std::is_const<T>::value, "Cannot make a reference to a const type.");
      static_assert(!std::is_same<Any, T>::value, "Cannot construct AnyRef to Any.");
      static_assert(!std::is_same<AnyRef, T>::value, "Cannot construct AnyRef to AnyRef.");
      static_assert(!std::is_same<AnyConstRef, T>::value, "Cannot construct AnyRef to AnyConstRef.");
    }

    AnyRef() {}
    AnyRef(const AnyRef& other) = default;
    AnyRef(Any& any) : type_info_(&any.type_info()), ref_(any.memory()) {}
    AnyRef& operator=(const AnyRef&) = default;

    const TypeInfo& type_info() const { return *type_info_; }

    template <class T> bool is_a() const;
    template <class T> Maybe<typename meta::RemoveConstRef<T>::Type &> get();
    template <class T> Maybe<const typename meta::RemoveConstRef<T>::Type &> get() const;
    template <class T, class F> auto when(F&& f) -> typename monad::Join<decltype(f(std::declval<T&>()))>::Type;
    template <class T, class F> auto when(F&& f) const -> typename monad::Join<decltype(f(std::declval<const T&>()))>::Type;
  private:
    const TypeInfo* type_info_ = &GetTypeInfo<NothingType>::Value;
    void* ref_ = nullptr;
    friend struct AnyConstRef;
  };

  struct AnyConstRef {
    template <class T>
    AnyConstRef(const T& object) : type_info_(&GetTypeInfo<T>::Value), ref_(reinterpret_cast<const void*>(&object)) {
      static_assert(!std::is_same<Any, T>::value, "Cannot construct AnyConstRef to Any.");
      static_assert(!std::is_same<AnyRef, T>::value, "Cannot construct AnyConstRef to AnyRef.");
      static_assert(!std::is_same<AnyConstRef, T>::value, "Cannot construct AnyConstRef to AnyConstRef.");
    }

    AnyConstRef() {}
    AnyConstRef(const AnyConstRef& other) = default;
    AnyConstRef(const Any& any) : type_info_(&any.type_info()), ref_(any.memory()) {}
    AnyConstRef(const AnyRef& ref) : type_info_(&ref.type_info()), ref_(ref.ref_) {}
    AnyConstRef& operator=(const AnyConstRef&) = default;

    const TypeInfo& type_info() const { return *type_info_; }

    template <class T> bool is_a() const;
    template <class T> Maybe<const typename meta::RemoveConstRef<T>::Type&> get() const;
    template <class T, class F> auto when(F&& f) const -> typename monad::Join<decltype(f(std::declval<const T&>()))>::Type;
  private:
    const TypeInfo* type_info_ = &GetTypeInfo<NothingType>::Value;
    const void* ref_ = nullptr;
  };

  template <class T>
  Any::Any(T&& object) : type_info_(&GetTypeInfo<typename meta::RemoveConstRef<T>::Type>::Value) {
    static_assert(!std::is_same<Any, T>::value, "Cannot construct Any containing Any.");
    static_assert(!std::is_same<AnyConstRef, T>::value, "Cannot construct Any containing AnyConstRef.");
    static_assert(!std::is_same<AnyRef, T>::value, "Cannot construct Any containing AnyRef.");
    ensure_allocation();
    type_info_->move_construct(memory(), reinterpret_cast<void*>(&object));
  }

  template <class T>
  Any::Any(const T& object) : type_info_(&GetTypeInfo<typename meta::RemoveConstRef<T>::Type>::Value) {
    static_assert(!std::is_same<AnyConstRef, T>::value, "Cannot construct Any containing AnyConstRef.");
    static_assert(!std::is_same<AnyRef, T>::value, "Cannot construct Any containing AnyRef.");
    ensure_allocation();
    type_info_->copy_construct(memory(), reinterpret_cast<const void*>(&object));
  }

  template <class T>
  bool Any::is_a() const {
    return type_info_ == &GetTypeInfo<T>::Value;
  }

  template <class T>
  Maybe<T> Any::get() const {
    // This supports getting the internals as a reference-Maybe with get<T&>()
    using Type = typename meta::RemoveConstRef<T>::Type;
    if (is_a<Type>()) {
      const Type& ref = *reinterpret_cast<const Type*>(memory());
      return Maybe<T>(ref);
    }
    return Nothing;
  }

  template <class T>
  Maybe<T> Any::get() {
    // This supports getting the internals as a reference-Maybe with get<T&>()
    using Type = typename meta::RemoveConstRef<T>::Type;
    if (is_a<Type>()) {
      Type& ref = *reinterpret_cast<Type*>(memory());
      return Maybe<T>(ref);
    }
    return Nothing;
  }

  template <class T, class F>
  auto Any::when(F&& f) -> typename monad::Join<decltype(f(std::declval<T>()))>::Type {
    return monad::fmap(get<T>(), std::forward<F>(f));
  }

  template <class T, class F>
  auto Any::when(F&& f) const -> typename monad::Join<decltype(f(std::declval<T>()))>::Type {
    return monad::fmap(get<T>(), std::forward<F>(f));
  }

  template <class T>
  bool AnyRef::is_a() const {
    return type_info_ == &GetTypeInfo<T>::Value;
  }

  template <class T>
  Maybe<const typename meta::RemoveConstRef<T>::Type &> AnyRef::get() const {
    // This supports getting the internals as a reference-Maybe with get<T&>()
    using Type = typename meta::RemoveConstRef<T>::Type;
    if (is_a<Type>()) {
      const Type& ref = *reinterpret_cast<const Type*>(ref_);
      return Maybe<const Type&>(ref);
    }
    return Nothing;
  }

  template <class T>
  Maybe<typename meta::RemoveConstRef<T>::Type &> AnyRef::get() {
    // This supports getting the internals as a reference-Maybe with get<T&>()
    using Type = typename meta::RemoveConstRef<T>::Type;
    if (is_a<Type>()) {
      Type& ref = *reinterpret_cast<Type*>(ref_);
      return Maybe<Type&>(ref);
    }
    return Nothing;
  }

  template <class T, class F>
  auto AnyRef::when(F&& f) -> typename monad::Join<decltype(f(std::declval<T&>()))>::Type {
    return monad::fmap(get<T>(), std::forward<F>(f));
  }

  template <class T, class F>
  auto AnyRef::when(F&& f) const -> typename monad::Join<decltype(f(std::declval<const T&>()))>::Type {
    return monad::fmap(get<T>(), std::forward<F>(f));
  }

  template <class T>
  bool AnyConstRef::is_a() const {
    return type_info_ == &GetTypeInfo<T>::Value;
  }

  template <class T>
  Maybe<const typename meta::RemoveConstRef<T>::Type &> AnyConstRef::get() const {
    using Type = typename meta::RemoveConstRef<T>::Type;
    if (is_a<Type>()) {
      const Type& ref = *reinterpret_cast<const Type*>(ref_);
      return Maybe<const Type&>(ref);
    }
    return Nothing;
  }

  template <class T, class F>
  auto AnyConstRef::when(F&& f) const -> typename monad::Join<decltype(f(std::declval<const T&>()))>::Type {
    return monad::fmap(get<T>(), std::forward<F>(f));
  }

}

#endif // WAYWARD_SUPPORT_ANY_HPP_INCLUDED

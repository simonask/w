#pragma once
#ifndef WAYWARD_SUPPORT_ANY_HPP_INCLUDED
#define WAYWARD_SUPPORT_ANY_HPP_INCLUDED

#include <wayward/support/type_info.hpp>
#include <wayward/support/meta.hpp>
#include <wayward/support/maybe.hpp>

namespace wayward {
  struct Any {
    Any();

    template <typename T>
    Any(T&&);

    Any(const Any&);
    Any(Any&&);
    Any(NothingType);
    ~Any();

    Any& operator=(const Any&);
    Any& operator=(Any&&);

    template <typename T>
    bool is_a() const;

    template <typename T, typename F>
    auto when(F&& f) -> typename monad::Join<Maybe<decltype(f(std::declval<T>()))>>::Type;
    template <typename T, typename F>
    auto when(F&& f) const -> typename monad::Join<Maybe<decltype(f(std::declval<const T>()))>>::Type;
    template <typename T>
    Maybe<T> get() const;

    const TypeInfo& type_info();

    void swap(Any& other);

  private:
    static const size_t InlineStorageThreshold = sizeof(void*) * 3;
    static const size_t InlineStorageAlignment = sizeof(void*);
    using InlineStorage = typename std::aligned_storage<InlineStorageThreshold, InlineStorageAlignment>::type;

    union {
      InlineStorage inline_storage_;
      void* heap_storage_;
    };
    const TypeInfo* type_info_ = &GetTypeInfo<NothingType>::Value;

    bool is_inline_storage() const {
      return type_info_ == nullptr || (type_info_->size <= InlineStorageThreshold && type_info_->alignment <= InlineStorageAlignment);
    }

    void destruct();
    void ensure_allocation();
    void* memory();
    const void* memory() const;
  };

  inline Any::Any() {}

  inline Any::Any(NothingType) {}

  template <typename T>
  Any::Any(T&& object) : type_info_(&GetTypeInfo<T>::Value) {
    using FundamentalType = typename meta::RemoveConstRef<T>::Type;
    ensure_allocation();
    new(memory()) T(std::forward<T>(object));
  }

  inline Any::Any(const Any& other) : type_info_(other.type_info_) {
    ensure_allocation();
    type_info_->copy_construct(memory(), other.memory());
  }

  inline Any::Any(Any&& other) : type_info_(other.type_info_) {
    ensure_allocation();
    type_info_->move_construct(memory(), other.memory());
  }

  inline Any::~Any() {
    destruct();
  }

  template <typename T>
  bool Any::is_a() const {
    return type_info_ == &GetTypeInfo<T>::Value;
  }

  template <typename T, typename F>
  auto Any::when(F&& f) -> typename monad::Join<Maybe<decltype(f(std::declval<T>()))>>::Type {
    return monad::fmap(get<T>(), std::forward<F>(f));
  }

  template <typename T, typename F>
  auto Any::when(F&& f) const -> typename monad::Join<Maybe<decltype(f(std::declval<const T>()))>>::Type {
    return monad::fmap(get<T>(), std::forward<F>(f));
  }

  template <typename T>
  Maybe<T> Any::get() const {
    if (is_a<T>()) {
      return *reinterpret_cast<const T*>(memory());
    }
    return Nothing;
  }

  inline void* Any::memory() {
    if (is_inline_storage()) {
      return reinterpret_cast<void*>(&inline_storage_);
    } else {
      return heap_storage_;
    }
  }

  inline const void* Any::memory() const {
    if (is_inline_storage()) {
      return reinterpret_cast<const void*>(&inline_storage_);
    } else {
      return heap_storage_;
    }
  }
}

#endif // WAYWARD_SUPPORT_ANY_HPP_INCLUDED

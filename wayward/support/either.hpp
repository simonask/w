#pragma once
#ifndef WAYWARD_SUPPORT_EITHER_HPP_INCLUDED
#define WAYWARD_SUPPORT_EITHER_HPP_INCLUDED

#include <wayward/support/meta.hpp>
#include <wayward/support/type_info.hpp>
#include <wayward/support/maybe.hpp>

#include <cassert>

namespace wayward {
  template <typename... Types_>
  struct Either {
    static const size_t NumTypes = sizeof...(Types_);
    static_assert(NumTypes < UINT8_MAX, "Too many possible types for Either! (max = 254)");

    using Self = Either<Types_...>;
    using Types = meta::TypeList<Types_...>;
    static const bool IsCopyConstructible = meta::AreAllCopyConstructible<Types>::Value;
    static const bool IsCopyAssignable = meta::AreAllCopyAssignable<Types>::Value;
    static const bool IsMoveConstructible = meta::AreAllMoveConstructible<Types>::Value;
    static const bool IsMoveAssignable = meta::AreAllMoveAssignable<Types>::Value;

    // Copy constructor with any valid type:
    template <typename T, typename = typename std::enable_if<meta::IndexOf<T, Types>::Value < NumTypes>::type>
    Either(const T& value) : type_index_(meta::IndexOf<T, Types>::Value) {
      new(memory()) T(value);
    }

    template <typename T, typename = typename std::enable_if<meta::IndexOf<T, Types>::Value < NumTypes>::type>
    Either(T&& value) : type_index_(meta::IndexOf<T, Types>::Value) {
      new(memory()) T(std::move(value));
    }

    Either(const Self& other) : type_index_(other.type_index_) {
      static_assert(IsCopyConstructible, "Cannot copy-construct this Either, because one or more of the represented types is not copy-constructible.");
      type_info().copy_construct(memory(), other.memory());
    }

    Either(Self&& other) : type_index_(other.type_index_) {
      static_assert(IsMoveConstructible, "Cannot move-construct this Either, because one or more of the represented types is not move-constructible.");
      type_info().move_construct(memory(), other.memory());
    }

    ~Either() {
      destruct();
    }

    template <typename T, typename = typename std::enable_if<IsCopyAssignable && meta::IndexOf<T, Types>::Value < NumTypes>::type>
    Self& operator=(const T& value) {
      auto new_index = meta::IndexOf<T, Types>::Value;
      if (type_index_ == new_index) {
        *memory_as<T>() = value;
      } else {
        destruct();
        type_index_ = new_index;
        new(memory()) T(value);
      }
      return *this;
    }

    template <typename T, typename = typename std::enable_if<IsMoveAssignable && meta::IndexOf<T, Types>::Value < NumTypes>::type>
    Self& operator=(T&& value) {
      auto new_index = meta::IndexOf<T, Types>::Value;
      if (type_index_ == new_index) {
        *memory_as<T>() = std::move(value);
      } else {
        destruct();
        type_index_ = new_index;
        new(memory()) T(std::move(value));
      }
      return *this;
    }

    Self& operator=(const Self& other) {
      static_assert(IsCopyAssignable, "Cannot copy-assign this Either, because one or more of the represented types is not copy-assignable.");
      if (type_index_ == other.type_index_) {
        type_info().copy_assign(memory(), other.memory());
      } else {
        destruct();
        type_index_ = other.type_index_;
        type_info().copy_construct(memory(), other.memory());
      }
      return *this;
    }

    Self& operator=(Self&& other) {
      static_assert(IsMoveAssignable, "Cannot move-assign this Either, because one or more of the represented types is not move-assignable.");
      if (type_index_ == other.type_index_) {
        type_info().move_assign(memory(), other.memory());
      } else {
        destruct();
        type_index_ = other.type_index_;
        type_info().move_construct(memory(), other.memory());
      }
      return *this;
    }

    template <typename T>
    typename std::enable_if<meta::IndexOf<T, Types>::Value < NumTypes, bool>::type
    is_a() const {
      return type_index_ == meta::IndexOf<T, Types>::Value;
    }

    template <typename T, typename F>
    typename std::enable_if<meta::IndexOf<T, Types>::Value < NumTypes>::type
    when(F f) {
      if (is_a<T>()) {
        f(*memory_as<T>());
      }
    }

    template <typename T, typename F>
    typename std::enable_if<meta::IndexOf<T, Types>::Value < NumTypes>::type
    when(F f) const {
      if (is_a<T>()) {
        f(*memory_as<T>());
      }
    }

    template <typename T>
    typename std::enable_if<meta::IndexOf<typename meta::RemoveConstRef<T>::Type, Types>::Value < NumTypes, Maybe<T>>::type
    get() & {
      using RawType = typename meta::RemoveConstRef<T>::Type;
      if (is_a<typename meta::RemoveConstRef<T>::Type>()) {
        return Maybe<T>{ *memory_as<RawType>() };
      }
      return Nothing;
    }

    template <typename T>
    typename std::enable_if<meta::IndexOf<typename meta::RemoveConstRef<T>::Type, Types>::Value < NumTypes, Maybe<T>>::type
    get() const & {
      using RawType = typename meta::RemoveConstRef<T>::Type;
      if (is_a<typename meta::RemoveConstRef<T>::Type>()) {
        return Maybe<T>{ *memory_as<RawType>() };
      }
      return Nothing;
    }

    template <typename T>
    typename std::enable_if<meta::IndexOf<typename meta::RemoveConstRef<T>::Type, Types>::Value < NumTypes, Maybe<typename meta::RemoveConstRef<T>::Type>>::type
    get() && {
      using RawType = typename meta::RemoveConstRef<T>::Type;
      if (is_a<RawType>()) {
        return Maybe<RawType>(std::move(*memory_as<RawType>()));
      }
      return Nothing;
    }

    void swap(Self& other);

    const TypeInfo& type_info() const {
      return *type_infos_[type_index_];
    }

    uint8_t which() const { return type_index_; }

  private:
    using Storage = typename std::aligned_storage<meta::MaxSize<Types>::Value, meta::MaxAlignment<Types>::Value>::type;
    Storage memory_;
    uint8_t type_index_;

    static constexpr const TypeInfo* const type_infos_[sizeof...(Types_)] = {&GetTypeInfo<Types_>::Value...};

    void* memory() { return reinterpret_cast<void*>(&memory_); }
    const void* memory() const { return reinterpret_cast<const void*>(&memory_); }

    template <typename T>
    T* memory_as() { return reinterpret_cast<T*>(memory()); }
    template <typename T>
    const T* memory_as() const { return reinterpret_cast<const T*>(memory()); }

    void destruct() {
      assert(type_index_ < NumTypes);
      type_info().destruct(memory());
      type_index_ = 255;
    }
  };

  template <typename... Types_>
  constexpr const TypeInfo* const Either<Types_...>::type_infos_[sizeof...(Types_)];
}

#endif // WAYWARD_SUPPORT_EITHER_HPP_INCLUDED

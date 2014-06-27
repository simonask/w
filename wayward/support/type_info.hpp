#pragma once
#ifndef WAYWARD_SUPPORT_TYPE_INFO_HPP_INCLUDED
#define WAYWARD_SUPPORT_TYPE_INFO_HPP_INCLUDED

#include <type_traits>
#include <typeinfo>
#include <string>

namespace wayward {
  std::string demangle_symbol(const std::string&);

  struct TypeInfo {
    using Constructor = void(*)(void*);
    using Destructor  = void(*)(void*);
    using CopyConstructor = void(*)(void*, const void*);
    using MoveConstructor = void(*)(void*, void*);
    using CopyAssign      = void(*)(void*, const void*);
    using MoveAssign      = void(*)(void*, void*);
    using GetID           = const std::type_info&(*)();

    size_t size;
    size_t alignment;
    Constructor construct;
    Destructor destruct;
    CopyConstructor copy_construct;
    MoveConstructor move_construct;
    CopyAssign copy_assign;
    MoveAssign move_assign;
    GetID get_id;

    std::string name() const { return demangle_symbol(get_id().name()); }
  };

  template <typename T>
  void construct(void* ptr) {
    new(ptr) T;
  }

  template <typename T>
  void destruct(void* memory) {
    reinterpret_cast<T*>(memory)->~T();
  }

  template <typename T>
  void copy_construct(void* a, const void* b) {
    new(a) T(*reinterpret_cast<const T*>(b));
  }

  template <typename T>
  void move_construct(void* a, void* b) {
    new(a) T(std::move(*reinterpret_cast<T*>(b)));
  }

  template <typename T>
  void copy_assign(void* a, const void* b) {
    *reinterpret_cast<T*>(a) = *reinterpret_cast<const T*>(b);
  }

  template <typename T>
  void move_assign(void* a, void* b) {
    *reinterpret_cast<T*>(a) = std::move(*reinterpret_cast<T*>(b));
  }

  template <typename T>
  const std::type_info& get_id() {
    return typeid(T);
  }

#define DEFINE_GET_FUNCTION_IF_SUPPORTED_BY_TYPE(CHECK_SUPPORT_STRUCT, FUNCTION_NAME) \
  template <typename T, bool IsSupported> struct GetFunctionIfSupportedImpl_ ## FUNCTION_NAME; \
  template <typename T> struct GetFunctionIfSupported_ ## FUNCTION_NAME { \
    static constexpr const auto Value = GetFunctionIfSupportedImpl_ ## FUNCTION_NAME <T, CHECK_SUPPORT_STRUCT<T>::value>::Value; \
  }; \
  template <typename T> struct GetFunctionIfSupportedImpl_ ## FUNCTION_NAME<T, true> { \
    static constexpr const auto Value = FUNCTION_NAME<T>; \
  }; \
  template <typename T> struct GetFunctionIfSupportedImpl_ ## FUNCTION_NAME<T, false> { \
    static constexpr const auto Value = nullptr; \
  } \


#define GET_FUNCTION_IF_SUPPORTED(T, FUNCTION_NAME) \
  GetFunctionIfSupported_ ## FUNCTION_NAME<T>::Value

  DEFINE_GET_FUNCTION_IF_SUPPORTED_BY_TYPE(std::is_constructible, construct);
  DEFINE_GET_FUNCTION_IF_SUPPORTED_BY_TYPE(std::is_copy_constructible, copy_construct);
  DEFINE_GET_FUNCTION_IF_SUPPORTED_BY_TYPE(std::is_copy_assignable, copy_assign);
  DEFINE_GET_FUNCTION_IF_SUPPORTED_BY_TYPE(std::is_move_constructible, move_construct);
  DEFINE_GET_FUNCTION_IF_SUPPORTED_BY_TYPE(std::is_move_assignable, move_assign);

  template <typename T>
  struct GetTypeInfo {
    static constexpr const TypeInfo Value = {
      .size           = sizeof(T),
      .alignment      = alignof(T),
      .construct      = GET_FUNCTION_IF_SUPPORTED(T, construct),
      .destruct       = wayward::destruct<T>,
      .copy_construct = GET_FUNCTION_IF_SUPPORTED(T, copy_construct),
      .move_construct = GET_FUNCTION_IF_SUPPORTED(T, move_construct),
      .copy_assign    = GET_FUNCTION_IF_SUPPORTED(T, copy_assign),
      .move_assign    = GET_FUNCTION_IF_SUPPORTED(T, move_assign),
      .get_id         = get_id<T>,
    };
  };

  template <typename T>
  constexpr const TypeInfo GetTypeInfo<T>::Value;
}

#endif // WAYWARD_SUPPORT_TYPE_INFO_HPP_INCLUDED

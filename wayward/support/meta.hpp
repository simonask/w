#pragma once
#ifndef WAYWARD_SUPPORT_META_HPP_INCLUDED
#define WAYWARD_SUPPORT_META_HPP_INCLUDED

#include <type_traits>
#include <limits.h>
#include <memory>

namespace wayward {
  namespace meta {
    struct TrueType { static const bool Value = true; };
    struct FalseType { static const bool Value = false; };

    template <class T> struct IsPointerLike { static const bool Value = std::is_pointer<T>::value; };
    template <class T> struct IsPointerLike<std::unique_ptr<T>> : TrueType {};
    template <class T> struct IsPointerLike<std::shared_ptr<T>> : TrueType {};

    constexpr size_t max(size_t a, size_t b) {
      return a < b ? b : a;
    }

    template <typename... Tx> struct TypeList {};

    template <typename T, typename TL> struct Contains;
    template <typename T>
    struct Contains<T, TypeList<>> {
      static constexpr bool Value = false;
    };
    template <typename T, typename Head, typename... Rest>
    struct Contains<T, TypeList<Head, Rest...>> {
      static constexpr bool Value = std::is_same<T, Head>::value || Contains<T, TypeList<Rest...>>::Value;
    };

    template <typename T, typename TL, size_t I = 0> struct IndexOf;
    template <typename T, size_t I>
    struct IndexOf<T, TypeList<>, I> {
      static constexpr size_t Value = SIZE_T_MAX;
    };
    template <typename T, size_t I, typename... Rest>
    struct IndexOf<T, TypeList<T, Rest...>, I> {
      static constexpr size_t Value = I;
    };
    template <typename T, size_t I, typename Head, typename... Rest>
    struct IndexOf<T, TypeList<Head, Rest...>, I> {
      static constexpr size_t Value = IndexOf<T, TypeList<Rest...>, I + 1>::Value;
    };

    template <typename T>
    struct RemoveConstRef {
      using Type = typename std::remove_const<typename std::remove_reference<T>::type>::type;
    };

    // === MaxSize ===
    template <typename T> struct MaxSize;
    template <> struct MaxSize<TypeList<>> {
      static const size_t Value = 0;
    };
    template <typename T, typename... Rest> struct MaxSize<TypeList<T, Rest...>> {
      static const size_t Value = max(sizeof(T), MaxSize<TypeList<Rest...>>::Value);
    };

    // === MaxAlignment ===
    template <typename T> struct MaxAlignment;
    template <> struct MaxAlignment<TypeList<>> {
      static const size_t Value = 0;
    };
    template <typename T, typename... Rest> struct MaxAlignment<TypeList<T, Rest...>> {
      static const size_t Value = max(alignof(T), MaxAlignment<TypeList<Rest...>>::Value);
    };

    // AllTypesSatisfyTrait_standard
    template <template <typename T> class Trait, typename Types> struct AllTypesSatifyTrait_standard;
    template <template <typename T> class Trait, typename First, typename... Rest>
    struct AllTypesSatifyTrait_standard<Trait, TypeList<First, Rest...>> {
      static const bool Value = Trait<First>::value && AllTypesSatifyTrait_standard<Trait, TypeList<Rest...>>::Value;
    };
    template <template <typename T> class Trait> struct AllTypesSatifyTrait_standard<Trait, TypeList<>> {
      static const bool Value = true;
    };
    // AnyTypesSatisfyTrait_standard
    template <template <typename T> class Trait, typename Types> struct AnyTypesSatifyTrait_standard;
    template <template <typename T> class Trait, typename First, typename... Rest>
    struct AnyTypesSatifyTrait_standard<Trait, TypeList<First, Rest...>> {
      static const bool Value = Trait<First>::value || AllTypesSatifyTrait_standard<Trait, TypeList<Rest...>>::Value;
    };
    template <template <typename T> class Trait> struct AnyTypesSatifyTrait_standard<Trait, TypeList<>> {
      static const bool Value = false;
    };

    // AreAllCopyConstructible
    template <typename Types> struct AreAllCopyConstructible {
      static const bool Value = AllTypesSatifyTrait_standard<std::is_copy_constructible, Types>::Value;
    };
    // AreAllCopyAssignable
    template <typename Types> struct AreAllCopyAssignable {
      static const bool Value = AllTypesSatifyTrait_standard<std::is_copy_assignable, Types>::Value;
    };
    // AreAllMoveConstructible
    template <typename Types> struct AreAllMoveConstructible {
      static const bool Value = AllTypesSatifyTrait_standard<std::is_move_constructible, Types>::Value;
    };
    // AreAllMoveAssignable
    template <typename Types> struct AreAllMoveAssignable {
      static const bool Value = AllTypesSatifyTrait_standard<std::is_move_assignable, Types>::Value;
    };
  }
}

#endif // WAYWARD_SUPPORT_META_HPP_INCLUDED

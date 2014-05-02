#pragma once
#ifndef WAYWARD_SUPPORT_META_HPP_INCLUDED
#define WAYWARD_SUPPORT_META_HPP_INCLUDED

#include <type_traits>

namespace wayward {
  namespace meta {
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
    struct IndexOf<T, TypeList<>, I> {}; // Does not exist.
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
  }
}

#endif // WAYWARD_SUPPORT_META_HPP_INCLUDED

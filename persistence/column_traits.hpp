#pragma once
#ifndef PERSISTENCE_COLUMN_TRAITS_HPP_INCLUDED
#define PERSISTENCE_COLUMN_TRAITS_HPP_INCLUDED

#include <type_traits>
#include <wayward/support/maybe.hpp>

namespace persistence {
  using wayward::Maybe;
  struct TrueType { static const bool Value = true; };
  struct FalseType { static const bool Value = false; };

  template <typename T> struct IsNumeric;
  template <typename T> struct IsNumeric<Maybe<T>> { static const bool Value = IsNumeric<T>::Value; };
  template <typename T> struct IsNumeric { static const bool Value = std::is_integral<T>::value || std::is_floating_point<T>::value; };

  template <typename T> struct IsString;
  template <> struct IsString<Maybe<std::string>>: TrueType {};
  template <> struct IsString<std::string>: TrueType {};
  template <typename T> struct IsString: FalseType {};

  template <typename T> struct IsBoolean;
  template <> struct IsBoolean<Maybe<bool>>: TrueType {};
  template <> struct IsBoolean<bool>: TrueType {};
  template <typename T> struct IsBoolean: FalseType {};

  template <typename T> struct IsNullable;
  template <typename T> struct IsNullable<Maybe<T>>: TrueType {};
  template <typename T> struct IsNullable: FalseType {};
}

#endif // PERSISTENCE_COLUMN_TRAITS_HPP_INCLUDED

#pragma once
#ifndef PERSISTENCE_COLUMN_ABILITIES_HPP_INCLUDED
#define PERSISTENCE_COLUMN_ABILITIES_HPP_INCLUDED

#include <persistence/relational_algebra.hpp>

namespace persistence {
  template <typename T>
  struct LiteralEqualityAbilities {
    using Cond = relational_algebra::Condition;
    Cond operator==(T lit);
    Cond operator!=(T lit);
  };

  template <typename T>
  struct LiteralOrderingAbilities : LiteralEqualityAbilities<T> {
    using Cond = relational_algebra::Condition;
    Cond operator<(T lit);
    Cond operator>(T lit);
    Cond operator<=(T lit);
    Cond operator>=(T lit);
  };

  template <typename T>
  struct NumericAbilities : LiteralOrderingAbilities<T> {};

  struct StringAbilities : LiteralEqualityAbilities<std::string> {
    using Cond = relational_algebra::Condition;

    Cond like(std::string cmp);
    Cond ilike(std::string cmp);
  };

  struct BooleanAbilities : LiteralEqualityAbilities<bool> {
    using Cond = relational_algebra::Condition;

    Cond is_true() &&;
    Cond is_not_true() &&;
    Cond is_false() &&;
    Cond is_not_false() &&;
    Cond is_unknown();
    Cond is_not_unknown();
  };

  template <typename T>
  struct NullableAbilities {
    using Cond = relational_algebra::Condition;

    Cond is_null();
    Cond is_not_null();
  };

  // Unless specialized, only allow raw SQL abilities.
  template <typename T> struct ColumnAbilities;

  // All "Maybe<T>" fields have the abilities of T plus Nullable.
  template <typename T>
  struct ColumnAbilities<Maybe<T>>: ColumnAbilities<T>, NullableAbilities<T> {};

  // Bind numeric abilities to everything that is a number.
  template <> struct ColumnAbilities<std::int32_t>: NumericAbilities<std::int32_t> {};
  template <> struct ColumnAbilities<std::int64_t>: NumericAbilities<std::int64_t> {};
  template <> struct ColumnAbilities<std::uint32_t>: NumericAbilities<std::uint32_t> {};
  template <> struct ColumnAbilities<std::uint64_t>: NumericAbilities<std::uint64_t> {};
  template <> struct ColumnAbilities<float>: NumericAbilities<float> {};
  template <> struct ColumnAbilities<double>: NumericAbilities<double> {};

  // Give string abilities to std::string.
  template <> struct ColumnAbilities<std::string>: StringAbilities {};

  // Give boolean abilities to bool.
  template <> struct ColumnAbilities<bool>: BooleanAbilities {};
}

#endif // PERSISTENCE_COLUMN_ABILITIES_HPP_INCLUDED

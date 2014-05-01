#pragma once
#ifndef PERSISTENCE_COLUMN_ABILITIES_HPP_INCLUDED
#define PERSISTENCE_COLUMN_ABILITIES_HPP_INCLUDED

#include <persistence/relational_algebra.hpp>
#include <wayward/support/maybe.hpp>

namespace persistence {

  template <typename Col, typename T>
  struct LiteralEqualityAbilities {
    using Cond = relational_algebra::Condition;
    Cond operator==(T lit) && { return std::move(*this).value() == relational_algebra::literal(std::move(lit)); }
    Cond operator!=(T lit) && { return std::move(*this).value() != relational_algebra::literal(std::move(lit)); }

  private:
    relational_algebra::Value value() && { return static_cast<Col*>(this)->value(); }
  };

  template <typename Col, typename T>
  struct LiteralOrderingAbilities : LiteralEqualityAbilities<Col, T> {
    using Cond = relational_algebra::Condition;
    Cond operator<(T lit) { return std::move(*this).value() < relational_algebra::literal(std::move(lit)); }
    Cond operator>(T lit) { return std::move(*this).value() > relational_algebra::literal(std::move(lit)); }
    Cond operator<=(T lit) { return std::move(*this).value() <= relational_algebra::literal(std::move(lit)); }
    Cond operator>=(T lit) { return std::move(*this).value() >= relational_algebra::literal(std::move(lit)); }

  private:
    relational_algebra::Value value() && { return static_cast<Col*>(this)->value(); }
  };

  template <typename Col, typename T>
  struct NumericAbilities : LiteralOrderingAbilities<Col, T> {};

  template <typename Col>
  struct StringAbilities : LiteralEqualityAbilities<Col, std::string> {
    using Cond = relational_algebra::Condition;

    Cond like(std::string cmp) && { return std::move(*this).value().like(std::move(cmp)); }
    Cond ilike(std::string cmp) && { return std::move(*this).value().ilike(std::move(cmp)); }

  private:
    relational_algebra::Value value() && { return static_cast<Col*>(this)->value(); }
  };

  template <typename Col>
  struct BooleanAbilities : LiteralEqualityAbilities<Col, bool> {
    using Cond = relational_algebra::Condition;

    Cond is_true() && { return std::move(*this).value().is_true(); }
    Cond is_not_true() && { return std::move(*this).value().is_not_true(); }
    Cond is_false() && { return std::move(*this).value().is_false(); }
    Cond is_not_false() && { return std::move(*this).value().is_not_false(); }
    Cond is_unknown() { return std::move(*this).value().is_unknown(); }
    Cond is_not_unknown() { return std::move(*this).value().is_not_unknown(); }

  private:
    relational_algebra::Value value() && { return static_cast<Col*>(this)->value(); }
  };

  template <typename Col, typename T>
  struct NullableAbilities {
    using Cond = relational_algebra::Condition;

    Cond is_null() { return std::move(*this).value().is_null(); }
    Cond is_not_null() { return std::move(*this).value().is_not_null(); }
  private:
    relational_algebra::Value value() && { return static_cast<Col*>(this)->value(); }
  };

  // Unless specialized, only allow raw SQL abilities.
  template <typename Col, typename T> struct ColumnAbilities;

  // All "Maybe<T>" fields have the abilities of T plus Nullable.
  template <typename Col, typename T>
  struct ColumnAbilities<Col, wayward::Maybe<T>>: ColumnAbilities<Col, T>, NullableAbilities<Col, T> {};

  // Bind numeric abilities to everything that is a number.
  template <typename Col> struct ColumnAbilities<Col, std::int32_t>: NumericAbilities<Col, std::int32_t> {};
  template <typename Col> struct ColumnAbilities<Col, std::int64_t>: NumericAbilities<Col, std::int64_t> {};
  template <typename Col> struct ColumnAbilities<Col, std::uint32_t>: NumericAbilities<Col, std::uint32_t> {};
  template <typename Col> struct ColumnAbilities<Col, std::uint64_t>: NumericAbilities<Col, std::uint64_t> {};
  template <typename Col> struct ColumnAbilities<Col, float>: NumericAbilities<Col, float> {};
  template <typename Col> struct ColumnAbilities<Col, double>: NumericAbilities<Col, double> {};

  // Give string abilities to std::string.
  template <typename Col> struct ColumnAbilities<Col, std::string>: StringAbilities<Col> {};

  // Give boolean abilities to bool.
  template <typename Col> struct ColumnAbilities<Col, bool>: BooleanAbilities<Col> {};
}

#endif // PERSISTENCE_COLUMN_ABILITIES_HPP_INCLUDED

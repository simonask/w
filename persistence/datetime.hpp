#pragma once
#ifndef PERSISTENCE_DATETIME_HPP_INCLUDED
#define PERSISTENCE_DATETIME_HPP_INCLUDED

#include <wayward/support/datetime.hpp>
#include <persistence/column_abilities.hpp>
#include <persistence/relational_algebra.hpp>
#include <persistence/type.hpp>

namespace persistence {
  struct DateTimeType : IDataTypeFor<wayward::DateTime> {
    std::string name() const final { return "DateTime"; }
    bool is_nullable() const final { return false; }
    bool deserialize_value(wayward::DateTime& value, const wayward::data_franca::ScalarSpectator& source) const final;
    bool serialize_value(const wayward::DateTime& value, wayward::data_franca::ScalarMutator& target) const final;
    bool has_value(const wayward::DateTime&) const final { return true; }
  };

  const DateTimeType* build_type(const TypeIdentifier<wayward::DateTime>*);

  // Enable use of values as SQL literals.
  namespace relational_algebra {
    template <>
    struct RepresentAsLiteral<wayward::DateTime> {
      static Value literal(const wayward::DateTime&);
    };
  }

  // Enable SQL comparison.
  template <typename Col> struct ColumnAbilities<Col, wayward::DateTime>: LiteralOrderingAbilities<Col, wayward::DateTime> {};
}

#endif // PERSISTENCE_DATETIME_HPP_INCLUDED

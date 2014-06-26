#pragma once
#ifndef PERSISTENCE_DATETIME_HPP_INCLUDED
#define PERSISTENCE_DATETIME_HPP_INCLUDED

#include <wayward/support/datetime.hpp>
#include <wayward/support/type.hpp>
#include <persistence/column_abilities.hpp>
#include <persistence/relational_algebra.hpp>

namespace persistence {
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

#pragma once
#ifndef PERSISTENCE_DATETIME_HPP_INCLUDED
#define PERSISTENCE_DATETIME_HPP_INCLUDED

#include <wayward/support/datetime.hpp>
#include <persistence/column_abilities.hpp>
#include <persistence/relational_algebra.hpp>
#include <wayward/support/datetime/type.hpp>

namespace persistence {
  // Enable SQL comparison.
  template <typename Col> struct ColumnAbilities<Col, wayward::DateTime>: LiteralOrderingAbilities<Col, wayward::DateTime> {};
}

#endif // PERSISTENCE_DATETIME_HPP_INCLUDED

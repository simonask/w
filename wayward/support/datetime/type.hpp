#pragma once
#ifndef WAYWARD_SUPPORT_DATETIME_TYPE_HPP_INCLUDED
#define WAYWARD_SUPPORT_DATETIME_TYPE_HPP_INCLUDED

#include <wayward/support/type.hpp>
#include <wayward/support/datetime/datetime.hpp>
#include <wayward/support/data_visitor.hpp>

namespace wayward {
  struct DateTimeType : DataTypeFor<DateTime> {
    std::string name() const final { return "DateTime"; }
    bool is_nullable() const final { return false; }
    void visit(DateTime& value, DataVisitor& visitor) const final { return visitor(value); }
    bool has_value(const wayward::DateTime&) const final { return true; }
  };

  const DateTimeType* build_type(const TypeIdentifier<DateTime>*);
}

#endif // WAYWARD_SUPPORT_DATETIME_TYPE_HPP_INCLUDED

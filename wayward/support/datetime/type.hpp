#pragma once
#ifndef WAYWARD_SUPPORT_DATETIME_TYPE_HPP_INCLUDED
#define WAYWARD_SUPPORT_DATETIME_TYPE_HPP_INCLUDED

#include <wayward/support/datetime/datetime.hpp>
#include <wayward/support/type.hpp>

namespace wayward {
  struct DateTimeType : IDataTypeFor<DateTime> {
    std::string name() const final { return "DateTime"; }
    bool is_nullable() const final { return false; }
    bool deserialize_value(DateTime& value, const data_franca::ScalarSpectator& source) const final;
    bool serialize_value(const DateTime& value, data_franca::ScalarMutator& target) const final;
    bool has_value(const DateTime&) const final { return true; }
  };

  const DateTimeType* build_type(const TypeIdentifier<DateTime>*);
}

#endif // WAYWARD_SUPPORT_DATETIME_TYPE_HPP_INCLUDED

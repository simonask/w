#pragma once
#ifndef PERSISTENCE_ASSIGN_ATTRIBUTES_HPP_INCLUDED
#define PERSISTENCE_ASSIGN_ATTRIBUTES_HPP_INCLUDED

#include <persistence/record_type.hpp>
#include <wayward/support/data_franca/spectator.hpp>

namespace persistence {
  template <typename Record>
  void assign_attributes(RecordPtr<Record> ptr, const wayward::data_franca::Spectator& data) {
    auto record_type = get_type<Record>();
    for (size_t i = 0; i < record_type->num_properties(); ++i) {
      auto prop = record_type->property_at(i);
      auto col = prop->column();
      if (data.has_key(col)) {
        prop->deserialize(*ptr, data[col]);
      }
    }
  }
}

#endif // PERSISTENCE_ASSIGN_ATTRIBUTES_HPP_INCLUDED

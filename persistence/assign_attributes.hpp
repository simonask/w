#pragma once
#ifndef PERSISTENCE_ASSIGN_ATTRIBUTES_HPP_INCLUDED
#define PERSISTENCE_ASSIGN_ATTRIBUTES_HPP_INCLUDED

#include <persistence/record_type.hpp>
#include <wayward/support/data_franca/spelunker.hpp>

namespace persistence {
  template <typename Record>
  void assign_attributes(RecordPtr<Record> ptr, const wayward::data_franca::Spelunker& data) {
    const IRecordType* record_type = get_type<Record>();

  }
}

#endif // PERSISTENCE_ASSIGN_ATTRIBUTES_HPP_INCLUDED

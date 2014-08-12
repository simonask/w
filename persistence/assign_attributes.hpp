#pragma once
#ifndef PERSISTENCE_ASSIGN_ATTRIBUTES_HPP_INCLUDED
#define PERSISTENCE_ASSIGN_ATTRIBUTES_HPP_INCLUDED

#include <persistence/record_type.hpp>
#include <wayward/support/data_franca/spectator.hpp>

namespace persistence {
  namespace detail {
    using wayward::AnyRef;

    void assign_attributes(AnyRef record, const IRecordType* record_type, const wayward::data_franca::Spectator& data);
  }

  template <typename Record>
  void assign_attributes(RecordPtr<Record> ptr, const wayward::data_franca::Spectator& data) {
    auto record_type = get_type<Record>();
    detail::assign_attributes(*ptr, record_type, data);
  }
}

#endif // PERSISTENCE_ASSIGN_ATTRIBUTES_HPP_INCLUDED

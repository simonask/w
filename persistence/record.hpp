#pragma once
#ifndef PERSISTENCE_RECORD_HPP_INCLUDED
#define PERSISTENCE_RECORD_HPP_INCLUDED

#include <persistence/record_type.hpp>

namespace persistence {
  template <typename T>
  bool is_persisted(const T& record) {
    auto pk_raw = get_type<T>()->primary_key();
    auto pk = dynamic_cast<const IPropertyOf<T>*>(pk_raw);
    if (pk == nullptr) {
      // TODO: Should we warn about this here?
      return false;
    }
    return pk->has_value(record);
  }

  template <typename T>
  bool is_new_record(const T& record) {
    return !is_persisted(record);
  }

  template <typename T>
  bool update(const T& record) {
    if (!is_persisted(record)) {
      // We can't update a record with no primary key.
      return false;
    }
  }

  template <typename T>
  bool insert(T& record) {
    // TODO: NIY
    return false;
  }

  template <typename T>
  bool save(T& record) {
    if (is_persisted(record)) {
      update(record);
    } else {
      insert(record);
    }
  }
}

#endif // PERSISTENCE_RECORD_HPP_INCLUDED

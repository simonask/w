#pragma once
#ifndef PERSISTENCE_RECORD_HPP_INCLUDED
#define PERSISTENCE_RECORD_HPP_INCLUDED

#include <persistence/record_type.hpp>
#include <persistence/property.hpp>
#include <persistence/connection_provider.hpp>
#include <persistence/datetime.hpp>
#include <persistence/insert.hpp>

#include <wayward/support/datetime.hpp>
#include <wayward/support/error.hpp>
#include <wayward/support/logger.hpp>

namespace persistence {
  struct PrimaryKeyError : wayward::Error {
    PrimaryKeyError(const std::string& msg) : wayward::Error(msg) {}
  };

  struct PersistError : wayward::Error {
    PersistError(const std::string& msg) : wayward::Error(msg) {}
  };

  template <typename T>
  bool is_persisted(const RecordPtr<T>& record) {
    auto pk = get_type<T>()->primary_key();
    if (pk == nullptr) {
      throw PrimaryKeyError{"This record class does not seem to have a primary key column defined."};
    }
    return pk->has_value(*record);
  }

  template <typename T>
  bool is_new_record(const RecordPtr<T>& record) {
    return !is_persisted(record);
  }

  template <typename T>
  bool update(const RecordPtr<T>& record) {
    if (!is_persisted(record)) {
      throw PrimaryKeyError{"Cannot UPDATE because the record is new and doesn't have a primary key."};
    }
    throw PersistError{"Update NIY"};
  }

  template <typename T>
  bool save(RecordPtr<T>& record) {
    if (is_persisted(record)) {
      update(record);
    } else {
      insert(record);
    }
    return false; // TODO
  }
}

#endif // PERSISTENCE_RECORD_HPP_INCLUDED

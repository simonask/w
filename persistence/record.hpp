#pragma once
#ifndef PERSISTENCE_RECORD_HPP_INCLUDED
#define PERSISTENCE_RECORD_HPP_INCLUDED

#include <persistence/record_type.hpp>
#include <persistence/property.hpp>

#include <wayward/support/error.hpp>

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
  bool insert(RecordPtr<T>& record) {
    if (is_persisted(record)) {
      throw PersistError{"Trying to insert record that already has a primary key."};
    }

    // Get the type and primary key.
    auto t = get_type<T>();
    auto pk = t->primary_key();

    // Set created_at if it exists.
    auto created_at_property = t->find_property_by_column_name("created_at");
    if (created_at_property != nullptr) {
      auto created_at_datetime_property = dynamic_cast<const PropertyOfBase<T, wayward::DateTime>*>(created_at_property);
      if (created_at_datetime_property != nullptr) {
        created_at_datetime_property->get(*record) = wayward::DateTime::now();
      }
    }

    // Build the INSERT query.
    ast::InsertQuery query;
    query.relation = t->relation();
    size_t num = t->num_properties();
    query.columns.reserve(num);
    query.values.reserve(num);
    if (pk) {
      query.returning_columns.push_back(pk->column());
    }

    auto conn = current_connection_provider().acquire_connection_for_data_store(t->data_store());

    for (size_t i = 0; i < t->num_properties(); ++i) {
      auto p = t->property_at(i);
      if (p == t->primary_key()) continue;
      query.columns.push_back(p->column());
      query.values.push_back(conn.literal_for_value(p->get_data(*record)));
    }

    conn.logger()->log(wayward::Severity::Debug, "p", wayward::format("Insert {0}", t->name()));

    auto result = conn.execute(query);

    // Set the primary key of the record.
    if (result) {
      auto mid = result->get(0, pk->column());
      if (mid) {
        wayward::data_franca::Mutator mut { pk->get_member_adapter(*record, wayward::data_franca::Options::None) };
        mut << *mid;
        return true;
      }
    }

    return false;
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

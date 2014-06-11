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
    auto pk_raw = get_type<T>()->primary_key();
    auto pk = dynamic_cast<const IPropertyOf<T>*>(pk_raw);
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
      throw PersistError{"Trying to insert record with a valid primary key."};
    }

    auto t = get_type<T>();
    auto pk_raw = t->primary_key();
    auto pk = dynamic_cast<const IPropertyOf<T>*>(pk_raw);

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
      auto p = &t->property_at(i);
      if (p == t->primary_key()) continue;
      auto pr = dynamic_cast<const IPropertyOf<T>*>(p);
      query.columns.push_back(p->column());
      query.values.push_back(conn.literal_for_value(pr->get_data(*record)));
    }

    conn.logger()->log(wayward::Severity::Debug, "p", wayward::format("Insert {0}", t->name()));

    auto result = conn.execute(query);

    if (result) {
      auto mid = result->get(0, pk->column());
      if (mid) {
        wayward::data_franca::Mutator mut { pk->get_member_adapter(*record) };
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

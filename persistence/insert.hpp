#pragma once
#ifndef PERSISTENCE_INSERT_HPP_INCLUDED
#define PERSISTENCE_INSERT_HPP_INCLUDED

#include <wayward/support/any.hpp>
#include <wayward/support/result.hpp>
#include <persistence/record_ptr.hpp>
#include <wayward/support/type.hpp>
#include <persistence/connection_pool.hpp>

namespace persistence {
  using wayward::AnyRef;
  using wayward::Result;

  struct IConnection;
  struct AcquiredConnection;
  struct IRecordType;
  struct IResultSet;
  namespace ast {
    struct InsertQuery;
  }

  namespace detail {
    struct InsertQueryWithConnection {
      ast::InsertQuery query;
      AcquiredConnection conn;
    };

    Result<InsertQueryWithConnection>
    make_insert_query(AnyRef record, const IRecordType* record_type, bool set_created_at);

    Result<std::unique_ptr<IResultSet>>
    execute_insert(const ast::InsertQuery& query, IConnection& connection, const IRecordType* record_type);

    Result<void>
    set_primary_key_from_results(AnyRef record, const IRecordType* record_type, const IResultSet& results);

    Result<void>
    insert(AnyRef record, const IRecordType* record_type, bool set_created_at);
  }


  template <class T>
  Result<void> insert(RecordPtr<T> record, bool set_created_at = true) {
    return detail::insert(*record, wayward::get_type<T>(), set_created_at);
  }
}

#endif // PERSISTENCE_INSERT_HPP_INCLUDED

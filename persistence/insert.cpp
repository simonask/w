#include "persistence/insert.hpp"
#include "persistence/ast.hpp"
#include "persistence/connection_pool.hpp"
#include "persistence/record_type.hpp"
#include "persistence/record.hpp"
#include "persistence/primary_key.hpp"
#include "persistence/data_as_literal.hpp"

#include <wayward/support/format.hpp>

namespace persistence {
  namespace detail {
    namespace monad = wayward::monad;

    using wayward::make_error;
    using wayward::NothingType;

    Result<InsertQueryWithConnection>
    make_insert_query(AnyRef record, const IRecordType* record_type, bool set_created_at) {
      //static_assert(!boost::is_copy_constructible<std::tuple<ast::InsertQuery, AcquiredConnection>>::value, "TUPLE IS is_copy_constructible!!!");

      // Get the primary key.
      auto pk = record_type->abstract_primary_key();

      if (pk) {
        Result<Any> existing_pk = pk->get(record);
        if (existing_pk.good()) {
          PrimaryKey& primary_key_value = *existing_pk.get().get<PrimaryKey&>();
          if (primary_key_value.is_persisted()) {
            return make_error<PersistError>(wayward::format("Trying to insert record that already has a primary key (real type: {0}).", existing_pk.get().type_info().name()));
          }
        }
      }

      // Set created_at if it exists.
      if (set_created_at) {
        auto created_at_property = record_type->find_abstract_property_by_column_name("created_at");
        if (created_at_property != nullptr) {
          auto now = wayward::DateTime::now();
          created_at_property->set(record, now);
        }
      }

      // Build the INSERT query.
      ast::InsertQuery query;
      query.relation = record_type->relation();
      size_t num = record_type->num_properties();
      query.columns.reserve(num);
      query.values.reserve(num);
      if (pk) {
        query.returning_columns.push_back(pk->column());
      }

      auto conn = current_connection_provider().acquire_connection_for_data_store(record_type->data_store());

      for (size_t i = 0; i < record_type->num_properties(); ++i) {
        auto p = record_type->abstract_property_at(i);
        if (p == pk) continue;
        query.columns.push_back(p->column());
        auto value = p->get(record);
        if (value) {
          DataAsLiteral data_as_literal;
          query.values.push_back(data_as_literal.make_literal(value.get(), &p->type()));
        } else {
          return std::move(std::move(value).error());
        }
      }

      return InsertQueryWithConnection{std::move(query), std::move(conn)};
    }

    Result<std::unique_ptr<IResultSet>>
    execute_insert(const ast::InsertQuery& query, IConnection& conn, const IRecordType* record_type) {
      conn.logger()->log(wayward::Severity::Debug, "p", wayward::format("Insert {0}", record_type->name()));
      std::unique_ptr<IResultSet> results;
      try {
        results = conn.execute(query);
      }
      catch (const wayward::Error& error) {
        conn.logger()->log(wayward::Severity::Error, "p", wayward::format("Error executing SQL:\n{0}", error.what()));
      }

      if (!results) {
        return make_error<PersistError>("Backend did not return any results (meaning INSERT probably failed).");
      }
      return std::move(results);
    }

    Result<void>
    set_primary_key_from_results(AnyRef record, const IRecordType* record_type, const IResultSet& results) {
      // Set the primary key of the record.
      auto pk = record_type->abstract_primary_key();
      if (pk) {
        auto id_as_string = results.get(0, pk->column());
        if (id_as_string) {
          PrimaryKey pk_value;
          std::stringstream ss { *id_as_string };
          ss >> pk_value.id;
          pk->set(record, pk_value);
          return Nothing;
        } else {
          return make_error<PersistError>("Backend did not return an ID for the primary key (meaning INSERT probably failed).");
        }
      }
      return Nothing;
    }

    Result<void>
    insert(AnyRef record, const IRecordType* record_type, bool set_created_at) {
      return monad::fmap(
        detail::make_insert_query(record, record_type, set_created_at),
        [&](InsertQueryWithConnection query_and_connection) {
          return monad::fmap(
            detail::execute_insert(
              query_and_connection.query,
              query_and_connection.conn,
              record_type
            ),
            [&](const std::unique_ptr<IResultSet>& results) {
              return detail::set_primary_key_from_results(record, record_type, *results);
            }
          );
        }
      );
    }
  }
}

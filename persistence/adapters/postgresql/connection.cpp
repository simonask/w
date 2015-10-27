#include "persistence/adapters/postgresql/connection.hpp"
#include "persistence/adapters/postgresql/renderers.hpp"
#include <libpq-fe.h>

#include <wayward/support/any.hpp>
#include <wayward/support/format.hpp>
#include <wayward/support/logger.hpp>
#include <wayward/support/data_franca/spectator.hpp>
#include <sstream>
#include <iostream>

namespace persistence {
  struct PostgreSQLConnection::Private {
    PGconn* conn = nullptr;
    std::shared_ptr<ILogger> logger;
  };

  static const AdapterRegistrar<PostgreSQLAdapter> registrar_ = AdapterRegistrar<PostgreSQLAdapter>("postgresql");

  std::unique_ptr<IConnection> PostgreSQLAdapter::connect(std::string connection_string) const {
    std::string error;
    auto conn = PostgreSQLConnection::connect(std::move(connection_string), &error);
    if (conn) {
      return std::move(conn);
    } else {
      throw PostgreSQLError{std::move(error)};
    }
  }

  namespace {
    struct PostgreSQLResultSet : IResultSet {
      explicit PostgreSQLResultSet(PGresult* result) : result(result) {}
      virtual ~PostgreSQLResultSet() {
        PQclear(result);
      }

      size_t width() const final { return PQnfields(result); }
      size_t height() const final { return PQntuples(result); }

      bool is_null_at(size_t row, const std::string& col) const final {
        auto idx = PQfnumber(result, col.c_str());
        return PQgetisnull(result, row, idx);
      }

      Maybe<std::string> get(size_t row, const std::string& col) const final {
        auto idx = PQfnumber(result, col.c_str());
        bool is_null = PQgetisnull(result, row, idx);
        if (!is_null) {
          return std::string{PQgetvalue(result, row, idx)};
        } else {
          return wayward::Nothing;
        }
      }

      std::vector<std::string> columns() const final {
        std::vector<std::string> r;
        size_t n = PQnfields(result);
        r.reserve(n);
        for (size_t i = 0; i < n; ++i) {
          r.push_back(std::string(PQfname(result, i)));
        }
        return r;
      }

      PGresult* result;
    };

    std::unique_ptr<PostgreSQLResultSet> make_results(PGresult* result) {
      return std::unique_ptr<PostgreSQLResultSet>(new PostgreSQLResultSet(result));
    }
  }

  PostgreSQLConnection::PostgreSQLConnection() : priv(new Private) {}

  PostgreSQLConnection::~PostgreSQLConnection() {
    PQfinish(priv->conn);
  }

  std::string
  PostgreSQLConnection::database() const {
    return PQdb(priv->conn);
  }

  std::string
  PostgreSQLConnection::user() const {
    return PQuser(priv->conn);
  }

  std::string
  PostgreSQLConnection::host() const {
    return PQhost(priv->conn);
  }

  std::shared_ptr<ILogger>
  PostgreSQLConnection::logger() const {
    return priv->logger;
  }

  void
  PostgreSQLConnection::set_logger(std::shared_ptr<ILogger> l) {
    priv->logger = std::move(l);
  }

  std::unique_ptr<IResultSet>
  PostgreSQLConnection::execute(std::string sql) {
    PGresult* results = PQexec(priv->conn, sql.c_str());
    priv->logger->log(wayward::Severity::Debug, "p", sql);
    switch (PQresultStatus(results)) {
      case PGRES_EMPTY_QUERY:
      case PGRES_COMMAND_OK:
      case PGRES_TUPLES_OK:
      case PGRES_COPY_OUT:
      case PGRES_COPY_IN:
      case PGRES_COPY_BOTH:
      case PGRES_SINGLE_TUPLE:
        return make_results(results);
      case PGRES_NONFATAL_ERROR:
        std::cerr << wayward::format("--> WARNING: {0}\n", PQresultErrorMessage(results));
        return make_results(results);
      case PGRES_BAD_RESPONSE:
      case PGRES_FATAL_ERROR:
        throw PostgreSQLError{std::string(PQresultErrorMessage(results))};
    }
  }

  namespace {
    struct DummyResolveSymbolicRelation : relational_algebra::IResolveSymbolicRelation {
      std::string relation_for_symbol(ast::SymbolicRelation rel) const final {
        throw relational_algebra::SymbolicRelationError("No resolver provided for query containing symbolic relations.");
      }
    };
  }

  std::unique_ptr<IResultSet>
  PostgreSQLConnection::execute(const ast::IQuery& query) {
    return this->execute(query, DummyResolveSymbolicRelation());
  }

  std::unique_ptr<IResultSet>
  PostgreSQLConnection::execute(const ast::IQuery& query, const relational_algebra::IResolveSymbolicRelation& rel) {
    std::string sql = to_sql(query, rel);
    return execute(std::move(sql));
  }

  std::string
  PostgreSQLConnection::to_sql(const ast::IQuery& query, const relational_algebra::IResolveSymbolicRelation& rel) {
    PostgreSQLQueryRenderer renderer(*this, rel);
    return query.to_sql(renderer);
  }

  std::string
  PostgreSQLConnection::to_sql(const ast::IQuery& query) {
    DummyResolveSymbolicRelation dummy;
    PostgreSQLQueryRenderer renderer(*this, dummy);
    return query.to_sql(renderer);
  }

  std::string
  PostgreSQLConnection::sanitize(std::string input) {
    const size_t max_len = input.size() * 2 + 1; // This size is expected by libpq.
    char buffer[max_len];
    int error;
    size_t len = PQescapeStringConn(priv->conn, buffer, input.c_str(), input.size(), &error);
    if (error == 0) {
      return std::string(buffer, len);
    } else {
      return "<UNKNOWN ERROR>";
    }
  }

  std::unique_ptr<PostgreSQLConnection>
  PostgreSQLConnection::connect(std::string connstr, std::string* out_error) {
    PGconn* conn = PQconnectdb(connstr.c_str());
    if (conn == nullptr) {
      if (out_error) *out_error = "Unknown error.";
      return nullptr;
    }

    if (PQstatus(conn) != CONNECTION_OK) {
      if (out_error) *out_error = PQerrorMessage(conn);
      PQfinish(conn);
      return nullptr;
    }

    auto p = new PostgreSQLConnection;
    p->priv->conn = conn;
    return std::unique_ptr<PostgreSQLConnection>(p);
  }
}

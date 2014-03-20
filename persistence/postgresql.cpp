#include <persistence/postgresql.hpp>
#include <persistence/postgresql_renderers.hpp>
#include <libpq-fe.h>

#include <wayward/support/format.hpp>
#include <sstream>

namespace persistence {
  struct PostgreSQLConnection::Private {
    PGconn* conn = nullptr;
  };

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

      std::string get(size_t row, const std::string& col) const final {
        auto idx = PQfnumber(result, col.c_str());
        return std::string{PQgetvalue(result, row, idx)};
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

  std::unique_ptr<IResultSet>
  PostgreSQLConnection::execute(std::string sql) {
    PGresult* results = PQexec(priv->conn, sql.c_str());
    // TODO: Check errors
    return make_results(results);
  }

  std::unique_ptr<IResultSet>
  PostgreSQLConnection::execute(const ast::IQuery& query) {
    std::string sql = to_sql(query);
    return execute(std::move(sql));
  }

  std::string
  PostgreSQLConnection::to_sql(const ast::IQuery& query) {
    PostgreSQLQueryRenderer renderer(*this);
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
      if (out_error) *out_error = "Connection failed.";
      PQfinish(conn);
      return nullptr;
    }

    auto p = new PostgreSQLConnection;
    p->priv->conn = conn;
    return std::unique_ptr<PostgreSQLConnection>(p);
  }
}

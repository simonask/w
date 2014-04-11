#pragma once
#ifndef PERSISTENCE_POSTGRESQL_HPP_INCLUDED
#define PERSISTENCE_POSTGRESQL_HPP_INCLUDED

#include <persistence/connection.hpp>
#include <persistence/adapter.hpp>
#include <stdexcept>

namespace persistence {
  struct PostgreSQLError : std::runtime_error {
    PostgreSQLError(const std::string& message) : std::runtime_error{message} {}
  };

  struct PostgreSQLAdapter : IAdapter {
    std::unique_ptr<IConnection> connect(std::string connection_string) const final;
  };

  struct PostgreSQLConnection : IConnection {
    virtual ~PostgreSQLConnection();

    // Info
    std::string database() const final;
    std::string user() const final;
    std::string host() const final;

    // Querying
    std::string sanitize(std::string sql_fragment) final;
    std::string to_sql(const ast::IQuery& query) final;
    std::unique_ptr<IResultSet> execute(const ast::IQuery& query) final;
    std::unique_ptr<IResultSet> execute(std::string sql) final;

    //
    static std::unique_ptr<PostgreSQLConnection>
    connect(std::string connection_string, std::string* out_error = nullptr);
  private:
    PostgreSQLConnection();
    struct Private;
    std::unique_ptr<Private> priv;
  };
}

#endif // PERSISTENCE_POSTGRESQL_HPP_INCLUDED

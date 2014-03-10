#pragma once
#ifndef PERSISTENCE_POSTGRESQL_HPP_INCLUDED
#define PERSISTENCE_POSTGRESQL_HPP_INCLUDED

#include <persistence/connection.hpp>

namespace persistence {
  struct PostgreSQLConnection : IConnection {
    virtual ~PostgreSQLConnection();

    // Info
    std::string database() const final;
    std::string user() const final;
    std::string host() const;

    // Querying
    std::unique_ptr<IResultSet> query(std::string sql) final;
    std::string sanitize(std::string sql_fragment) final;

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

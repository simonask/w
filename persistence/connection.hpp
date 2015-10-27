#pragma once
#ifndef PERSISTENCE_CONNECTION_HPP_INCLUDED
#define PERSISTENCE_CONNECTION_HPP_INCLUDED

#include <persistence/result_set.hpp>

#include <persistence/ast.hpp>

#include <memory>

namespace wayward {
  struct ILogger;
  struct AnyConstRef;

  namespace data_franca {
    struct Spectator;
  }
}

namespace persistence {
  using wayward::AnyConstRef;

  namespace ast {
    struct IQuery;
  }
  using wayward::ILogger;

  namespace relational_algebra {
    struct IResolveSymbolicRelation;
  }

  struct IConnection {
    virtual ~IConnection() {}

    // Info
    virtual std::string database() const = 0;
    virtual std::string user() const = 0;
    virtual std::string host() const = 0;

    virtual std::shared_ptr<ILogger> logger() const = 0;
    virtual void set_logger(std::shared_ptr<ILogger> l) = 0;

    // Queries
    virtual std::string to_sql(const ast::IQuery& q) = 0;
    virtual std::string to_sql(const ast::IQuery& q, const relational_algebra::IResolveSymbolicRelation&) = 0;
    virtual std::string sanitize(std::string input) = 0;
    virtual std::unique_ptr<IResultSet> execute(std::string sql) = 0;
    virtual std::unique_ptr<IResultSet> execute(const ast::IQuery& query) = 0;
    virtual std::unique_ptr<IResultSet> execute(const ast::IQuery& query, const relational_algebra::IResolveSymbolicRelation&) = 0;
  };

  void set_connection(IConnection* conn);
  IConnection& get_connection();
}

#endif // PERSISTENCE_CONNECTION_HPP_INCLUDED

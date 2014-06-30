#pragma once
#ifndef PERSISTENCE_CONNECTION_POOL_HPP_INCLUDED
#define PERSISTENCE_CONNECTION_POOL_HPP_INCLUDED

#include <persistence/connection.hpp>
#include <wayward/support/maybe.hpp>
#include <wayward/support/error.hpp>
#include <wayward/support/any.hpp>

namespace persistence {
  struct IConnectionPool;
  struct IAdapter;
  using wayward::Maybe;

  struct AcquiredConnection : IConnection {
    virtual ~AcquiredConnection() { release(); }
    AcquiredConnection() {}
    AcquiredConnection(AcquiredConnection&&);
    AcquiredConnection(IConnectionPool& pool, IConnection& connection) : pool_(&pool), connection_(&connection) {}
    AcquiredConnection& operator=(AcquiredConnection&&);

    // IConnection interface
    std::string database() const final { return connection_->database(); }
    std::string user() const final { return connection_->user(); }
    std::string host() const final { return connection_->host(); }
    std::string to_sql(const ast::IQuery& q) final { return connection_->to_sql(q); }
    std::string to_sql(const ast::IQuery& q, const relational_algebra::IResolveSymbolicRelation& rel) final { return connection_->to_sql(q, rel); }
    std::string sanitize(std::string input) final { return connection_->sanitize(std::move(input)); }
    std::unique_ptr<IResultSet> execute(std::string sql) final { return connection_->execute(std::move(sql)); }
    std::unique_ptr<IResultSet> execute(const ast::IQuery& query) final { return connection_->execute(query); }
    std::unique_ptr<IResultSet> execute(const ast::IQuery& query, const relational_algebra::IResolveSymbolicRelation& rel) final { return connection_->execute(query, rel); }
    std::shared_ptr<ILogger> logger() const final { return connection_->logger(); }
    void set_logger(std::shared_ptr<ILogger> l) final { connection_->set_logger(std::move(l)); }
  private:
    AcquiredConnection(const AcquiredConnection&) = delete;
    IConnectionPool* pool_ = nullptr;
    IConnection* connection_ = nullptr;
    void release();
  };

  struct IConnectionPool {
    virtual ~IConnectionPool() {}
    virtual Maybe<AcquiredConnection> try_acquire() = 0;
    virtual AcquiredConnection acquire() = 0;
  protected:
    friend struct AcquiredConnection;
    virtual void release(IConnection*) = 0;
  };

  struct ConnectionPoolError : wayward::Error {
    ConnectionPoolError(const std::string& str) : wayward::Error(str) {}
  };

  std::unique_ptr<IConnectionPool> make_limited_connection_pool(const IAdapter& adapter, std::string connection_string, size_t pool_size);
}

#endif // PERSISTENCE_CONNECTION_POOL_HPP_INCLUDED

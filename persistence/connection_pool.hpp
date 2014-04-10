#pragma once
#ifndef PERSISTENCE_CONNECTION_POOL_HPP_INCLUDED
#define PERSISTENCE_CONNECTION_POOL_HPP_INCLUDED

#include <persistence/connection.hpp>
#include <wayward/support/maybe.hpp>

#include <mutex>

namespace persistence {
  class ConnectionPool;
  struct IAdapter;
  using wayward::Maybe;

  struct AcquiredConnection : IConnection {
    virtual ~AcquiredConnection() { release(); }
    AcquiredConnection(AcquiredConnection&&);

    // IConnection interface
    std::string database() const final { return connection_->database(); }
    std::string user() const final { return connection_->user(); }
    std::string host() const final { return connection_->host(); }
    std::string to_sql(const ast::IQuery& q) final { return connection_->to_sql(q); }
    std::string sanitize(std::string input) final { return connection_->sanitize(std::move(input)); }
    std::unique_ptr<IResultSet> execute(std::string sql) final { return connection_->execute(std::move(sql)); }
    std::unique_ptr<IResultSet> execute(const ast::IQuery& query) final { return connection_->execute(query); }
  private:
    friend class ConnectionPool;
    AcquiredConnection(ConnectionPool& pool, IConnection& connection) : pool_(&pool), connection_(&connection) {}
    ConnectionPool* pool_ = nullptr;
    IConnection* connection_ = nullptr;
    void release();
  };

  class ConnectionPool {
  public:
    ConnectionPool(const IAdapter& adapter, std::string connection_string, size_t pool_size) : adapter_(adapter), connection_string_(std::move(connection_string)), size_(pool_size) { fill_pool(); }

    Maybe<AcquiredConnection> try_acquire();
    AcquiredConnection acquire();
  private:
    const IAdapter& adapter_;
    std::mutex mutex_;
    std::string connection_string_;
    std::vector<std::unique_ptr<IConnection>> pool_;
    size_t size_ = 0;
    size_t acquired_ = 0;

    void fill_pool();
  };
}

#endif // PERSISTENCE_CONNECTION_POOL_HPP_INCLUDED

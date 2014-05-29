#include <persistence/connection_pool.hpp>
#include <persistence/adapter.hpp>
#include <wayward/support/format.hpp>

#include <mutex>
#include <condition_variable>

namespace persistence {
  using wayward::Nothing;

  struct LimitedConnectionPool : IConnectionPool {
  public:
    LimitedConnectionPool(const IAdapter& adapter, std::string connection_string, size_t pool_size) : adapter_(adapter), connection_string_(std::move(connection_string)), size_(pool_size) { fill_pool(); }
    virtual ~LimitedConnectionPool();

    Maybe<AcquiredConnection> try_acquire();
    AcquiredConnection acquire();
  private:
    const IAdapter& adapter_;
    std::mutex mutex_;
    std::condition_variable available_;

    std::string connection_string_;
    std::vector<std::unique_ptr<IConnection>> pool_;
    std::vector<std::unique_ptr<IConnection>> reserved_;
    size_t size_ = 0;

    void fill_pool();
    AcquiredConnection acquire_unlocked();
    friend struct AcquiredConnection;
    void release(IConnection*);
  };

  std::unique_ptr<IConnectionPool> make_limited_connection_pool(const IAdapter& adapter, std::string connection_string, size_t pool_size) {
    return std::unique_ptr<IConnectionPool>(new LimitedConnectionPool(adapter, std::move(connection_string), pool_size));
  }

  LimitedConnectionPool::~LimitedConnectionPool() {
    if (reserved_.size()) {
      throw ConnectionPoolError(wayward::format("Consistency error: Trying to destroy connection pool, but there are {0} AcquiredConnections still in use.", reserved_.size()));
    }
  }

  void LimitedConnectionPool::fill_pool() {
    pool_.reserve(size_);
    reserved_.reserve(size_);
    while (pool_.size() < size_) {
      pool_.push_back(adapter_.connect(connection_string_));
    }
  }

  AcquiredConnection LimitedConnectionPool::acquire() {
    std::unique_lock<std::mutex> L(mutex_);
    while (pool_.empty()) {
      available_.wait(L);
    }
    return acquire_unlocked();
  }

  Maybe<AcquiredConnection> LimitedConnectionPool::try_acquire() {
    std::lock_guard<std::mutex> L(mutex_);
    if (pool_.empty()) {
      return Nothing;
    }
    return acquire_unlocked();
  }

  AcquiredConnection LimitedConnectionPool::acquire_unlocked() {
    auto conn = std::move(pool_.back());
    pool_.pop_back();
    AcquiredConnection ac{*this, *conn.get()};
    reserved_.push_back(std::move(conn));
    return std::move(ac);
  }

  void LimitedConnectionPool::release(IConnection* conn) {
    std::lock_guard<std::mutex> L(mutex_);
    auto it = std::find_if(reserved_.begin(), reserved_.end(), [&](const std::unique_ptr<IConnection>& ptr) { return ptr.get() == conn; });
    if (it != reserved_.end()) {
      std::swap(*it, reserved_.back());
      auto c = std::move(reserved_.back());
      reserved_.pop_back();
      pool_.push_back(std::move(c));
      available_.notify_one();
    } else {
      throw ConnectionPoolError("Tried to release a connection that wasn't reserved by this connection pool!");
    }
  }

  AcquiredConnection::AcquiredConnection(AcquiredConnection&& other) {
    std::swap(pool_, other.pool_);
    std::swap(connection_, other.connection_);
  }

  AcquiredConnection& AcquiredConnection::operator=(AcquiredConnection&& other) {
    std::swap(pool_, other.pool_);
    std::swap(connection_, other.connection_);
    return *this;
  }

  void AcquiredConnection::release() {
    if (connection_)
      pool_->release(connection_);
  }
}

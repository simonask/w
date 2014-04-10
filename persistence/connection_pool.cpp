#include <persistence/connection_pool.hpp>
#include <persistence/adapter.hpp>
#include <wayward/support/format.hpp>

namespace persistence {
  using wayward::Nothing;

  void ConnectionPool::fill_pool() {
    pool_.reserve(size_);
    reserved_.reserve(size_);
    while (pool_.size() < size_) {
      pool_.push_back(adapter_.connect(connection_string_));
    }
  }

  AcquiredConnection ConnectionPool::acquire() {
    std::unique_lock<std::mutex> L(mutex_);
    L.lock();
    while (pool_.empty()) {
      available_.wait(L);
    }
    return acquire_unlocked();
  }

  Maybe<AcquiredConnection> ConnectionPool::try_acquire() {
    std::lock_guard<std::mutex> L(mutex_);
    if (pool_.empty()) {
      return Nothing;
    }
    return acquire_unlocked();
  }

  AcquiredConnection ConnectionPool::acquire_unlocked() {
    auto conn = std::move(pool_.back());
    pool_.pop_back();
    AcquiredConnection ac{*this, *conn.get()};
    reserved_.push_back(std::move(conn));
    return std::move(ac);
  }

  void ConnectionPool::release(IConnection* conn) {
    std::lock_guard<std::mutex> L(mutex_);
    auto it = std::find_if(reserved_.begin(), reserved_.end(), [&](const std::unique_ptr<IConnection>& ptr) { return ptr.get() == conn; });
    if (it != reserved_.end()) {
      std::swap(*it, reserved_.back());
      auto c = std::move(reserved_.back());
      reserved_.pop_back();
      pool_.push_back(std::move(c));
      available_.notify_one();
    } else {
      throw ConnectionPoolError("Tried to release a connection that wasn't reserved by this ConnectionPool!");
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

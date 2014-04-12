#include <persistence/connection_retainer.hpp>
#include <persistence/connection_pool.hpp>
#include <persistence/data_store.hpp>
#include <wayward/support/format.hpp>

#include <map>
#include <algorithm>

namespace persistence {
  struct ConnectionRetainer::Impl : IConnectionPool {
    Impl(IConnectionProvider& previous, std::vector<std::string> data_store_names) : previous_(previous), necessary_data_stores_(std::move(data_store_names)) {
      // Sorting data stores by name to ensure that we're always requesting locks on them
      // in the same order. This should minimize the chance of a deadlock happening.
      std::sort(necessary_data_stores_.begin(), necessary_data_stores_.end());
    }

    IConnectionProvider& previous_;

    std::vector<std::string> necessary_data_stores_;
    std::map<std::string, AcquiredConnection> acquired_connections_;
    size_t loaned_out_ = 0;

    bool is_aquired() const;
    void acquire_all();
    void release_all();

    // Returns true if lock was acquired AND all subsequent locks were acquired.
    bool acquire_at_index_recursive(size_t idx, bool block);

    // IConnectionPool interface
    Maybe<AcquiredConnection> try_acquire() final;
    AcquiredConnection acquire() final;
    void release(IConnection*) final;
  };

  ConnectionRetainer::ConnectionRetainer(IConnectionProvider& previous, std::vector<std::string> data_store_names) : impl(new ConnectionRetainer::Impl(previous, std::move(data_store_names))) {

  }

  ConnectionRetainer::~ConnectionRetainer() {
    release_all();
  }

  AcquiredConnection ConnectionRetainer::acquire_connection_for_data_store(const std::string& data_store_name) {
    impl->acquire_all();
    auto it = impl->acquired_connections_.find(data_store_name);
    if (it != impl->acquired_connections_.end()) {
      ++impl->loaned_out_;
      return AcquiredConnection{*impl, it->second};
    }
    throw ConnectionRetainerError{wayward::format("Data store '{0}' not supported by this ConnectionRetainer.", data_store_name)};
  }

  void ConnectionRetainer::release_all() {
    impl->release_all();
  }

  bool ConnectionRetainer::Impl::is_aquired() const {
    return acquired_connections_.size() == necessary_data_stores_.size();
  }

  void ConnectionRetainer::Impl::release_all() {
    if (loaned_out_ > 0) {
      throw ConnectionRetainerError{wayward::format("Consistency error: Attempting to release connection from ConnectionRetainer, but there are still connections in use.")};
    }
    acquired_connections_.clear();
  }

  void ConnectionRetainer::Impl::acquire_all() {
    // Keep trying until we have them all.
    while (!acquire_at_index_recursive(0, true));
  }

  bool ConnectionRetainer::Impl::acquire_at_index_recursive(size_t idx, bool block) {
    if (idx >= necessary_data_stores_.size()) return true;
    auto& name = necessary_data_stores_.at(idx);
    AcquiredConnection acquired_connection;

    if (block) {
      acquired_connection = data_store(name).acquire();
    } else {
      auto mconn = data_store(name).try_acquire();
      if (mconn) {
        acquired_connection = std::move(*mconn);
      } else {
        return false;
      }
    }

    if (acquire_at_index_recursive(idx+1, false)) {
      acquired_connections_[name] = std::move(acquired_connection);
      return true;
    } else {
      return false;
    }
  }

  Maybe<AcquiredConnection> ConnectionRetainer::Impl::try_acquire() {
    return wayward::Nothing;
  }

  AcquiredConnection ConnectionRetainer::Impl::acquire() {
    return AcquiredConnection();
  }

  void ConnectionRetainer::Impl::release(IConnection* connection) {
    --loaned_out_;
  }
}

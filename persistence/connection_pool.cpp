#include <persistence/connection_pool.hpp>
#include <persistence/adapter.hpp>

namespace persistence {
  void ConnectionPool::fill_pool() {
    pool_.reserve(size_);
    while (pool_.size() < size_) {
      pool_.push_back(adapter_.connect(connection_string_));
    }
  }
}

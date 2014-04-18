#pragma once
#ifndef PERSISTENCE_DATA_STORE_HPP_INCLUDED
#define PERSISTENCE_DATA_STORE_HPP_INCLUDED

#include <string>

#include <persistence/connection_pool.hpp>
#include <wayward/support/logger.hpp>
#include <wayward/support/error.hpp>

namespace persistence {
  using wayward::ILogger;

  struct DataStoreOptions {
    size_t pool_size = 5;
    std::shared_ptr<ILogger> logger = nullptr; // Uses console output if left as null.
  };

  struct DataStoreError : wayward::Error {
    DataStoreError(const std::string& message) : wayward::Error(message) {}
  };

  class DataStore {
  public:
    DataStore(std::string name, std::string connection_url, const DataStoreOptions& options);

    AcquiredConnection acquire();
    Maybe<AcquiredConnection> try_acquire();

    const std::shared_ptr<ILogger>& logger();
    void set_logger(std::shared_ptr<ILogger>);
  private:
    std::string name;
    std::string connection_url;
    std::shared_ptr<ILogger> logger_;
    std::unique_ptr<IConnectionPool> pool;

    const IAdapter& adapter_or_error(const std::string& connection_url);
  };

  void setup(std::string data_store_name, std::string connection_url, const DataStoreOptions& options = DataStoreOptions{});
  void setup(std::string connection_url, const DataStoreOptions& options = DataStoreOptions{});

  DataStore& data_store();
  DataStore& data_store(const std::string& name);
}

#endif // PERSISTENCE_DATA_STORE_HPP_INCLUDED

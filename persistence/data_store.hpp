#pragma once
#ifndef PERSISTENCE_DATA_STORE_HPP_INCLUDED
#define PERSISTENCE_DATA_STORE_HPP_INCLUDED

#include <string>

#include <persistence/connection_pool.hpp>

namespace persistence {
  struct DataStoreOptions {
    size_t pool_size = 5;
  };

  struct DataStoreError : std::runtime_error {
    DataStoreError(const std::string& message) : std::runtime_error(message) {}
  };

  class DataStore {
  public:
    DataStore(std::string name, std::string connection_url, const DataStoreOptions& options);
  private:
    std::string name;
    std::string connection_url;
    ConnectionPool pool;

    const IAdapter& adapter_or_error(const std::string& connection_url);
  };

  void setup(std::string data_store_name, std::string connection_url, const DataStoreOptions& options = DataStoreOptions{});
  void setup(std::string connection_url, const DataStoreOptions& options = DataStoreOptions{});

  DataStore& data_store();
  DataStore& data_store(const std::string& name);
}

#endif // PERSISTENCE_DATA_STORE_HPP_INCLUDED

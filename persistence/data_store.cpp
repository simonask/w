#include <persistence/data_store.hpp>
#include <wayward/support/format.hpp>
#include <wayward/support/uri.hpp>
#include <persistence/adapter.hpp>

#include <map>
#include <iostream>

namespace persistence {
  using wayward::URI;
  using wayward::ILogger;

  DataStore::DataStore(std::string name, std::string connection_url, const DataStoreOptions& options)
  : name(std::move(name))
  , connection_url(connection_url)
  , logger_(options.logger)
  , pool(make_limited_connection_pool(adapter_or_error(connection_url), connection_url, options.pool_size))
  {}

  AcquiredConnection DataStore::acquire() {
    auto conn = pool->acquire();
    conn.set_logger(logger());
    return conn;
  }

  Maybe<AcquiredConnection> DataStore::try_acquire() {
    auto mconn = pool->try_acquire();
    if (mconn) {
      mconn->set_logger(logger());
    }
    return mconn;
  }

  const std::shared_ptr<ILogger>& DataStore::logger() {
    if (logger_ == nullptr) {
      logger_ = wayward::ConsoleStreamLogger::get();
    }
    return logger_;
  }

  void DataStore::set_logger(std::shared_ptr<ILogger> l) {
    logger_ = std::move(l);
  }

  const IAdapter& DataStore::adapter_or_error(const std::string& connection_url) {
    auto maybe_uri = URI::parse(connection_url);
    if (!maybe_uri) {
      throw DataStoreError(wayward::format("Invalid data store URI: {0}", connection_url));
    }
    auto adapter = adapter_for_protocol(maybe_uri->scheme);
    if (adapter) {
      return *adapter;
    }
    throw DataStoreError(wayward::format("No adapter found for protocol '{0}'.", maybe_uri->scheme));
  }

  namespace {
    std::map<std::string, std::unique_ptr<DataStore>> g_data_stores;
  }

  void setup(std::string data_store_name, std::string connection_url, const DataStoreOptions& options) {
    g_data_stores[data_store_name] = std::unique_ptr<DataStore>(new DataStore{data_store_name, connection_url, options});
  }

  void setup(std::string connection_url, const DataStoreOptions& options) {
    setup("default", std::move(connection_url), options);
  }

  DataStore& data_store() {
    return data_store("default");
  }

  DataStore& data_store(const std::string& name) {
    auto it = g_data_stores.find(name);
    if (it != g_data_stores.end()) {
      return *it->second;
    } else {
      throw DataStoreError(wayward::format("Data store \"{0}\" is not configured. Call persistence::setup(\"{0}\", \"<connection string>\") to set it up.", name));
    }
  }
}

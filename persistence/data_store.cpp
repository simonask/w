#include <persistence/data_store.hpp>
#include <wayward/support/format.hpp>
#include <wayward/support/uri.hpp>
#include <persistence/adapter.hpp>

#include <map>

namespace persistence {
  using wayward::URI;

  DataStore::DataStore(std::string name, std::string connection_url, const DataStoreOptions& options)
  : name(std::move(name))
  , connection_url(connection_url)
  , pool(adapter_or_error(connection_url), connection_url, options.pool_size)
  {}

  AcquiredConnection DataStore::acquire() {
    return pool.acquire();
  }

  Maybe<AcquiredConnection> DataStore::try_acquire() {
    return pool.try_acquire();
  }

  const IAdapter& DataStore::adapter_or_error(const std::string& connection_url) {
    URI uri{connection_url};
    std::string protocol = uri.scheme();
    auto adapter = adapter_for_protocol(protocol);
    if (adapter) {
      return *adapter;
    }
    throw DataStoreError(wayward::format("No adapter found for protocol '{0}'.", protocol));
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

#include <persistence/connection_provider.hpp>
#include <persistence/connection_pool.hpp>
#include <persistence/data_store.hpp>

namespace persistence {
  AcquiredConnection DefaultConnectionProvider::acquire_connection_for_data_store(const std::string& data_store_name) {
    auto& ds = data_store(data_store_name);
    return ds.acquire();
  }

  namespace {
    static DefaultConnectionProvider g_default_connection_provider = DefaultConnectionProvider();
    __thread IConnectionProvider* g_current_connection_provider = &g_default_connection_provider;
  }

  IConnectionProvider& current_connection_provider() {
    return *g_current_connection_provider;
  }

  void with_connection_provider(IConnectionProvider& provider, std::function<void()> callback) {
    struct PopConnectionProvider {
      IConnectionProvider* previous;
      PopConnectionProvider(IConnectionProvider* connection_provider) : previous(connection_provider) {}
      ~PopConnectionProvider() {
        g_current_connection_provider = previous;
      }
    };

    PopConnectionProvider pop{g_current_connection_provider};
    g_current_connection_provider = &provider;
    callback();
  }
}

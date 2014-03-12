#include "persistence/p.hpp"
#include <persistence/postgresql.hpp>

namespace persistence {
  namespace {
    static std::unique_ptr<IConnection> g_connection; // TODO: Use a connection pool or *something*
  }

  bool connect(const Configuration& config, std::string* out_error) {
    g_connection = PostgreSQLConnection::connect(config.connection_string, out_error);
    return g_connection != nullptr;
  }

  IConnection& get_connection() {
    return *g_connection;
  }
}

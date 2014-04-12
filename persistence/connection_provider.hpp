#pragma once
#ifndef PERSISTENCE_CONNECTION_PROVIDER_HPP_INCLUDED
#define PERSISTENCE_CONNECTION_PROVIDER_HPP_INCLUDED

#include <string>
#include <functional>

namespace persistence {
  struct AcquiredConnection;

  struct IConnectionProvider {
    virtual AcquiredConnection acquire_connection_for_data_store(const std::string& data_store_name) = 0;
  };

  /*
    This just does a plain datastore->acquire.
  */
  struct DefaultConnectionProvider : IConnectionProvider {
    AcquiredConnection acquire_connection_for_data_store(const std::string& data_store_name) final;
  };

  IConnectionProvider& current_connection_provider();
  void with_connection_provider(IConnectionProvider& provider, std::function<void()> callback);
}

#endif // PERSISTENCE_CONNECTION_PROVIDER_HPP_INCLUDED

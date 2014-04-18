#pragma once
#ifndef PERSISTENCE_CONNECTION_RETAINER_HPP_INCLUDED
#define PERSISTENCE_CONNECTION_RETAINER_HPP_INCLUDED

#include <persistence/connection_provider.hpp>
#include <wayward/support/error.hpp>

#include <vector>
#include <string>

namespace persistence {
  /*
    ConnectionRetainer provides support for the following use case:
    - A number of data store connections are required to satisfy a query.
    - Multiple threads may be competing for access to the same connection pools.
    - Several queries to the same data store are necessary within a short time (say, serving a web request),
      but acquiring a data store connection is expensive (lots of locking).

    ConnectionRetainer solves this problem by providing a mechanism to specify which
    data stores are needed in a specific context. The connections will then all be
    acquired as soon as one is needed, in a deadlock-safe manner. The connections will be
    kept and reused for the duration of the lifetime of the ConnectionRetainer, or until
    release_all is called.

    If no data store connections are requested, none will be acquired.
  */
  struct ConnectionRetainer : IConnectionProvider {
    // IConnectionProvider interface
    AcquiredConnection acquire_connection_for_data_store(const std::string& data_store_name);

    // ConnectionRetainer interface
    explicit ConnectionRetainer(IConnectionProvider& previous, std::vector<std::string> data_store_names);
    ~ConnectionRetainer();
    void release_all();
  private:
    struct Impl;
    std::unique_ptr<Impl> impl;
  };

  struct ConnectionRetainerError : wayward::Error {
    ConnectionRetainerError(const std::string& message) : wayward::Error(message) {}
  };
}

#endif // PERSISTENCE_CONNECTION_RETAINER_HPP_INCLUDED

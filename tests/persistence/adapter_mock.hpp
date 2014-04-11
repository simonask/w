#pragma once
#ifndef PERSISTENCE_TEST_ADAPTER_MOCK_HPP_INCLUDED
#define PERSISTENCE_TEST_ADAPTER_MOCK_HPP_INCLUDED

#include <persistence/adapter.hpp>
#include "connection_mock.hpp"

namespace persistence {
  namespace test {
    struct AdapterMock : IAdapter {
      ConnectionMock connection;

      std::unique_ptr<IConnection> connect(std::string conn_url) const {
        return std::unique_ptr<IConnection>(new ConnectionMock(connection));
      }
    };
  }
}

#endif // PERSISTENCE_TEST_ADAPTER_MOCK_HPP_INCLUDED

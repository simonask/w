#pragma once
#ifndef PERSISTENCE_TEST_ADAPTER_MOCK_HPP_INCLUDED
#define PERSISTENCE_TEST_ADAPTER_MOCK_HPP_INCLUDED

#include <persistence/adapter.hpp>
#include "connection_mock.hpp"

namespace persistence {
  namespace test {
    struct AdapterMock : IAdapter {
      ConnectionMock connection;
      std::shared_ptr<ResultSetMock> result_set_;

      AdapterMock() {
        result_set_ = std::make_shared<ResultSetMock>();
      }

      std::unique_ptr<IConnection> connect(std::string conn_url) const {
        auto conn = std::unique_ptr<ConnectionMock>(new ConnectionMock(connection));
        conn->results_ = result_set_;
        return std::move(conn);
      }
    };
  }
}

#endif // PERSISTENCE_TEST_ADAPTER_MOCK_HPP_INCLUDED

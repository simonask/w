#pragma once
#ifndef PERSISTENCE_TEST_CONNECTION_MOCK_HPP_INCLUDED
#define PERSISTENCE_TEST_CONNECTION_MOCK_HPP_INCLUDED

#include <persistence/connection.hpp>
#include <persistence/postgresql_renderers.hpp>

#include <regex>

#include "result_set_mock.hpp"

namespace persistence {
  namespace test {
    struct ConnectionMock : persistence::IConnection {
      // Info
      std::string database() const override { return database_; }
      std::string user() const override { return user_; }
      std::string host() const override { return host_; }

      // Queries
      std::string to_sql(const ast::IQuery& q) override;
      std::string sanitize(std::string input) override;
      std::unique_ptr<IResultSet> execute(std::string sql) override;
      std::unique_ptr<IResultSet> execute(const ast::IQuery& query) override;

      std::string database_;
      std::string user_;
      std::string host_;
      ResultSetMock results_;

      size_t sanitize_called = 0;
      size_t to_sql_called   = 0;
      size_t execute_called_with_string = 0;
      size_t execute_called_with_query  = 0;

    private:
      std::string to_sql_impl(const ast::IQuery& q);
      std::string sanitize_impl(std::string input);
      std::unique_ptr<IResultSet> execute_impl(std::string sql);
    };

    inline std::string ConnectionMock::to_sql(const ast::IQuery& q) {
      ++to_sql_called;
      return to_sql_impl(q);
    }

    inline std::string ConnectionMock::sanitize(std::string input) {
      ++sanitize_called;
      return sanitize_impl(input);
    }

    inline std::unique_ptr<IResultSet> ConnectionMock::execute(std::string sql) {
      ++execute_called_with_string;
      return execute_impl(sql);
    }

    inline std::unique_ptr<IResultSet> ConnectionMock::execute(const ast::IQuery& query) {
      ++execute_called_with_query;
      return execute_impl(to_sql_impl(query));
    }

    inline std::string ConnectionMock::to_sql_impl(const ast::IQuery& q) {
      PostgreSQLQueryRenderer renderer(*this);
      return q.to_sql(renderer);
    }

    inline std::string ConnectionMock::sanitize_impl(std::string input) {
      std::regex r {"'"};
      return std::regex_replace(input, r, "\\'");
    }

    inline std::unique_ptr<IResultSet> ConnectionMock::execute_impl(std::string sql) {
      return std::unique_ptr<IResultSet>(new ResultSetMock(results_));
    }
  }
}

#endif // PERSISTENCE_TEST_CONNECTION_MOCK_HPP_INCLUDED

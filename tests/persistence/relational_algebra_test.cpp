#include <gtest/gtest.h>

#include <persistence/relational_algebra.hpp>

#include "connection_mock.hpp"

namespace {
  using namespace persistence::relational_algebra;
  using namespace persistence::test;

  struct RelationalAlgebraWithConnectionMock : ::testing::Test {
    ConnectionMock connection;
  };

  TEST_F(RelationalAlgebraWithConnectionMock, select_star_from_foos) {
    auto query = projection("foos");
    auto sql = connection.to_sql(*query.query);
    EXPECT_EQ(sql, "SELECT * FROM foos");
  }

  TEST_F(RelationalAlgebraWithConnectionMock, select_star_from_foos_where_a_equals_2) {
    auto query = projection("foos").where(column("foos", "a") == literal(2));
    auto sql = connection.to_sql(*query.query);
    EXPECT_EQ(sql, "SELECT * FROM foos WHERE \"foos\".\"a\" = 2");
  }

  TEST_F(RelationalAlgebraWithConnectionMock, select_b_from_foos_where_a_greater_than_2) {
    auto query = projection("foos").where(column("foos", "a") > literal(2)).select({column("foos", "b")});
    auto sql = connection.to_sql(*query.query);
    EXPECT_EQ(sql, "SELECT \"foos\".\"b\" FROM foos WHERE \"foos\".\"a\" > 2");
  }

  TEST_F(RelationalAlgebraWithConnectionMock, select_star_from_foos_order_by_a) {
    auto query = projection("foos").order({column("foos", "a")});
    auto sql = connection.to_sql(*query.query);
    EXPECT_EQ(sql, "SELECT * FROM foos ORDER BY \"foos\".\"a\"");
  }

  TEST_F(RelationalAlgebraWithConnectionMock, select_star_from_foos_order_by_a_desc) {
    auto query = projection("foos").order({column("foos", "a")}).reverse_order();
    auto sql = connection.to_sql(*query.query);
    EXPECT_EQ(sql, "SELECT * FROM foos ORDER BY \"foos\".\"a\" DESC");
  }
}

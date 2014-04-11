#include <gtest/gtest.h>

#include <persistence/projection.hpp>
#include <persistence/primary_key.hpp>
#include <persistence/persistence_macro.hpp>
#include <persistence/types.hpp>

#include "connection_mock.hpp"

namespace {
  using persistence::PrimaryKey;
  using wayward::Maybe;
  using wayward::Nothing;
  using persistence::from;
  using persistence::column;

  using persistence::test::ConnectionMock;

  struct Foo {
    PrimaryKey id;

    std::string string_value;
    Maybe<std::string> nullable_string_value;
    int32_t int32_value;
    double double_value;
  };

  PERSISTENCE(Foo) {
    property(&Foo::id, "id");
    property(&Foo::string_value, "string_value");
    property(&Foo::nullable_string_value, "nullable_string_value");
    property(&Foo::int32_value, "int32_value");
    property(&Foo::double_value, "double_value");
  }

  struct ProjectionWithConnectionMock : ::testing::Test {
    ConnectionMock connection;

    void SetUp() override {

      connection.results_.columns_ = {"t0_c0", "t0_c1", "t0_c2", "t0_c3", "t0_c4"};
      for (size_t i = 0; i < 5; ++i) {
        std::vector<Maybe<std::string>> row {
          wayward::format("{0}", i+1),
          wayward::format("String {0}", i),
          (i % 2 == 0 ? Maybe<std::string>(wayward::format("Nullable String {0}", i)) : Maybe<std::string>(Nothing)),
          wayward::format("{0}", (int32_t)(i*2)),
          wayward::format("{0}", ((double)i * 123.4))
        };
        connection.results_.rows_.push_back(std::move(row));
      }
    }
  };

  TEST_F(ProjectionWithConnectionMock, maps_primary_key) {
    auto q = from<Foo>();
    size_t counter = 0;
    q.each([&](const Foo& foo) {
      EXPECT_EQ(foo.id, counter+1);
      ++counter;
    });
  }

  TEST_F(ProjectionWithConnectionMock, maps_string_value) {
    auto q = from<Foo>();
    size_t counter = 0;
    q.each([&](const Foo& foo) {
      EXPECT_EQ(foo.string_value, *connection.results_.rows_.at(counter).at(1));
      ++counter;
    });
  }

  TEST_F(ProjectionWithConnectionMock, maps_nullable_string_value) {
    auto q = from<Foo>();
    size_t counter = 0;
    q.each([&](const Foo& foo) {
      EXPECT_EQ((bool)foo.nullable_string_value, (bool)connection.results_.rows_.at(counter).at(2));
      ++counter;
    });
  }

  TEST_F(ProjectionWithConnectionMock, maps_int32_value) {
    auto q = from<Foo>();
    size_t counter = 0;
    q.each([&](const Foo& foo) {
      std::stringstream ss;
      ss.str(*connection.results_.rows_.at(counter).at(3));
      int32_t n;
      ss >> n;
      EXPECT_EQ(foo.int32_value, n);
      ++counter;
    });
  }

  TEST_F(ProjectionWithConnectionMock, maps_double_value) {
    auto q = from<Foo>();
    size_t counter = 0;
    q.each([&](const Foo& foo) {
      std::stringstream ss;
      ss.str(*connection.results_.rows_.at(counter).at(4));
      double n;
      ss >> n;
      EXPECT_EQ(foo.double_value, n);
      ++counter;
    });
  }
}

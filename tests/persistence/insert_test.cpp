#include <gtest/gtest.h>

#include <persistence/record.hpp>
#include <persistence/datetime.hpp>
#include <persistence/data_store.hpp>
#include <persistence/primary_key.hpp>
#include <persistence/persistence_macro.hpp>

#include "connection_mock.hpp"
#include "adapter_mock.hpp"

namespace {
  using persistence::PrimaryKey;
  using persistence::Context;
  using wayward::DateTime;
  using wayward::Maybe;
  using wayward::Nothing;

  using namespace persistence::test;

  struct Foo {
    PrimaryKey id;
    DateTime created_at;
    Maybe<DateTime> updated_at;
    int integer_value = -1;
    std::string string_value;
    Maybe<std::string> nullable_string_value;
  };

  PERSISTENCE(Foo) {
    property(&Foo::id, "id");
    property(&Foo::created_at, "created_at");
    property(&Foo::updated_at, "updated_at");
    property(&Foo::integer_value, "integer_value");
    property(&Foo::string_value, "string_value");
    property(&Foo::nullable_string_value, "nullable_string_value");
  }

  struct InsertionTest : ::testing::Test {
    persistence::AdapterRegistrar<AdapterMock> adapter_registrar_ = "test";
    Context context;

    persistence::test::ResultSetMock& results() {
      return *adapter_registrar_.adapter_.result_set_;
    }

    void SetUp() override {
      persistence::setup("test://test");
    }
  };

  TEST_F(InsertionTest, generates_sql_to_insert) {
    Foo foo;
    auto tuple = persistence::detail::make_insert_query(foo, persistence::get_type<Foo>(), false);
    if (!tuple.good()) {
      throw *tuple.error();
    }
    EXPECT_EQ(true, tuple.good());
    auto q = std::move(tuple.get().query);
    auto sql = tuple.get().conn.to_sql(q);
    EXPECT_EQ(0, sql.find("INSERT INTO foos ("));
  }
}

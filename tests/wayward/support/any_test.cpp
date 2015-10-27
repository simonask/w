#include <gtest/gtest.h>

#include <wayward/support/any.hpp>

namespace {
  using wayward::Any;
  using wayward::AnyRef;
  using wayward::AnyConstRef;

  TEST(Any, stores_integer) {
    Any any { (int)123 };
    EXPECT_EQ(true, any.is_a<int>());
    auto m = any.get<int>();
    EXPECT_EQ(123, *m);
  }

  TEST(Any, accesses_contents_by_reference) {
    Any any { (int)123 };
    auto m = any.get<int&>();
    *m = 456;
    EXPECT_EQ(456, *any.get<int>());
  }

  TEST(AnyRef, stores_integer_ref) {
    int n = 123;
    AnyRef any { n };
    EXPECT_EQ(true, any.is_a<int>());
    auto m = any.get<int>();
    EXPECT_EQ(n, *m);
  }

  TEST(AnyRef, accesses_contents_by_reference) {
    int n = 123;
    AnyRef any { n };
    auto m = any.get<int>();
    *m = 456;
    EXPECT_EQ(456, n);
  }
}

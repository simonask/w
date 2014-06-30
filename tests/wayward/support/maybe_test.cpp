#include <gtest/gtest.h>

#include <wayward/support/maybe.hpp>

namespace {
  using wayward::Maybe;
  using wayward::NothingType;
  using wayward::Nothing;

  TEST(Maybe, encapsulates_int) {
    Maybe<int> m = 123;
    EXPECT_EQ(123, *m);
  }

  TEST(Maybe, encapsulates_int_ref) {
    int n = 123;
    Maybe<int&> m { n };
    EXPECT_EQ(123, *m);
    when_maybe(m, [&](int& x) {
      x = 456;
    });
    EXPECT_EQ(456, n);
  }
}

#include <gtest/gtest.h>
#include <wayward/support/monad.hpp>
#include <wayward/support/maybe.hpp>

namespace {
  using wayward::Maybe;
  using wayward::Just;
  using wayward::Nothing;
  namespace monad = wayward::monad;

  TEST(fmap, fmap_maybe) {
    auto add3 = [](int x) { return x + 3; };

    Maybe<int> a = 2;
    auto m = monad::fmap(a, add3);
    EXPECT_EQ(5, *m);

    Maybe<int> b = Nothing;
    auto mb = monad::fmap(b, add3);
    EXPECT_EQ(mb, Nothing);
  }

  TEST(fmap, fmap_maybe_in_chain) {
    Maybe<int> a = 2;
    Maybe<int> b = 3;
    auto m = monad::fmap(a, [&](int a_) {
      return monad::fmap(b, [&](int b_) {
        return a_ + b_;
      });
    });
    EXPECT_EQ(*m, 5);
  }

  TEST(lift, lift_maybes) {
    auto add = [](int x, int y) { return x + y; };

    Maybe<int> a = 2;
    Maybe<int> b = 3;
    auto m = monad::lift(a, b, add);
    EXPECT_EQ(*m, 5);

    Maybe<int> c = 123;
    Maybe<int> d = Nothing;
    auto mb = monad::lift(c, d, add);
    EXPECT_EQ(mb, Nothing);
  }

  TEST(lift, lift_maybes_with_different_types) {
    auto append_number_as_string = [](std::string x, int n) {
      std::stringstream ss;
      ss << x << n;
      return ss.str();
    };

    auto a = Just(std::string{"Hello "});
    Maybe<int> b = 123;
    auto m = monad::lift(a, b, append_number_as_string);
    EXPECT_EQ(*m, "Hello 123");

    Maybe<std::string> c; // Is nothing.
    Maybe<int> d = 123;
    auto mb = monad::lift(c, d, append_number_as_string);
    EXPECT_EQ(mb, Nothing);
  }

}

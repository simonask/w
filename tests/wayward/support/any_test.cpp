#include <gtest/gtest.h>

#include <wayward/support/any.hpp>

namespace {
  using wayward::Any;
  using wayward::NothingType;
  using wayward::Nothing;

  TEST(Any, empty_is_a_nothing_type) {
    Any empty;
    EXPECT_EQ(true, empty.is_a<NothingType>());
  }

  TEST(Any, constructs_with_Nothing) {
    Any empty { Nothing };
    EXPECT_EQ(true, empty.is_a<NothingType>());
  }

  TEST(Any, constructs_with_int) {
    Any number { (int)123 };
    EXPECT_EQ(true, number.is_a<int>());
  }

  struct DestructionTest {
    bool& destroyed;
    DestructionTest(bool& destroyed) : destroyed(destroyed) { destroyed = false; }
    ~DestructionTest() { destroyed = true; }
  };

  TEST(Any, destroys_contents) {
    bool destroyed = false;
    {
      Any d { DestructionTest{destroyed} };
    }
    EXPECT_EQ(true, destroyed);
  }

  TEST(Any, accesses_internals_with_when) {
    Any n { (int)123 };
    int contents = 0;
    n.when<int>([&](int m) {
      contents = m;
    });
    EXPECT_EQ(123, contents);
  }

  TEST(Any, accesses_internals_with_get) {
    Any n { (int)123 };
    auto contents = n.get<int>();
    EXPECT_EQ(true, (bool)contents);
    EXPECT_EQ(123, *contents);
  }

  TEST(Any, behaves_as_monad_with_lift) {
    Any n { (int)123 };
    Any s { std::string{"Hello, World!"} };
    bool ran = false;
    wayward::monad::lift(n.get<int>(), s.get<std::string>(), [&](int m, std::string z) {
      ran = true;
      EXPECT_EQ(123, m);
      EXPECT_EQ("Hello, World!", z);
    });
    EXPECT_EQ(true, ran);
  }

  // TODO: Test move construction, copy construction, move assignment, copy assignment.
}

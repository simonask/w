#include <gtest/gtest.h>
#include <wayward/support/either.hpp>

namespace {
  using wayward::Either;

  TEST(Either, with_int_and_string) {
    Either<int, std::string> either { 1 };
    EXPECT_EQ(0, either.which());
    either = std::string{"Hello, World!"};
    EXPECT_EQ(1, either.which());
  }

  TEST(Either, copy_assignment) {
    Either<int, std::string> a { 1 };
    Either<int, std::string> b { std::string{"Hello, World"} };
    a = b;
    EXPECT_EQ(1, a.which());
  }
}

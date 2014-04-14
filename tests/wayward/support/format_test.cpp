#include <gtest/gtest.h>
#include <wayward/support/format.hpp>

namespace {
  using wayward::format;

  TEST(format_with_index, interpolates_identity) {
    std::string str = "Hello, World!";
    auto a = format("{0}", str);
    EXPECT_EQ(a, str);
  }

  TEST(format_with_index, interpolates_simple) {
    std::string str = "World";
    auto a = format("Hello, {0}!", str);
    EXPECT_EQ(a, "Hello, World!");
  }

  TEST(format_with_index, interpolates_same_value_twice) {
    auto a = format("{0}{1}{0}", "a", "b");
    EXPECT_EQ(a, "aba");
  }

  TEST(format_with_index, does_not_interpolate_malformed_key) {
    auto a = format("{0a}", "a");
    EXPECT_EQ(a, "{0a}");
  }

  TEST(format_with_index, does_not_interpolate_index_out_of_bounds) {
    auto a = format("{100}", "a");
    EXPECT_EQ(a, "{100}");
  }

  TEST(format_with_dictionary, interpolates_identity) {
    std::string str = "Hello, World!";
    auto a = format("{message}", {{"message", str}});
    EXPECT_EQ(a, str);
  }

  TEST(format_with_dictionary, interpolates_simple) {
    std::string str = "World";
    auto a = format("Hello, {message}!", {{"message", str}});
    EXPECT_EQ(a, "Hello, World!");
  }

  TEST(format_with_dictionary, interpolates_same_value_twice) {
    auto a = format("{a}{b}{a}", {{"a", "a"}, {"b", "b"}});
    EXPECT_EQ(a, "aba");
  }

  TEST(format_with_dictionary, does_not_interpolate_non_existent_key) {
    auto a = format("{b}", "a");
    EXPECT_EQ(a, "{b}");
  }

  // TODO: Support number formatting strings, like {0:09.2f} == "%09.2f"
}

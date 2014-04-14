#include <gtest/gtest.h>
#include <wayward/support/json.hpp>

namespace {
  using wayward::as_json;
  using wayward::JSONMode;

  TEST(json, convert_integer) {
    auto result = as_json(123);
    EXPECT_EQ(result, "123");
  }

  TEST(json, escapes_double_quoted_strings) {
    auto result = as_json("\"Hello, World!\"");
    EXPECT_EQ(result, "\"\\\"Hello, World!\\\"\"");
  }

  TEST(json, convert_float) {
    auto result = as_json(123.45);
    EXPECT_EQ(result, "123.45");
  }

  TEST(json, convert_vector_of_floats) {
    std::vector<float> numbers {12.3, 45.6, 78.9};
    auto result = as_json(numbers, JSONMode::Compact);
    EXPECT_EQ(result, "[12.3, 45.6, 78.9]");
  }

  TEST(json, convert_map_of_string_to_integer) {
    std::map<std::string, int> map {{"a", 12}, {"b", 34}, {"c", 56}};
    auto result = as_json(map, JSONMode::Compact);
    EXPECT_EQ(result, "{\"a\": 12, \"b\": 34, \"c\": 56}");
  }

  // TEST(json, does_not_render_lists_with_a_trailing_comma)
  // TEST(json, does_not_render_dictionaries_with_a_trailing_comma)
}

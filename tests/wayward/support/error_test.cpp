#include <gtest/gtest.h>

#include <wayward/support/error.hpp>
#include <regex>

namespace {
  using wayward::Error;

  void __attribute__((noinline))
  a_function_that_throws() {
    throw Error("Hello, World!");
  }

  TEST(Error, generates_a_backtrace) {
    try {
      a_function_that_throws();
    } catch (const Error& err) {
      bool any_contains_function_name = false;
      for (auto& line: err.backtrace()) {
        if (line.find("a_function_that_throws") != std::string::npos) {
          any_contains_function_name = true;
          break;
        }
      }
      EXPECT_EQ(true, any_contains_function_name);
    } catch (...) {
      EXPECT_EQ("", "An exception other than wayward::Error was thrown.");
    }
  }
}

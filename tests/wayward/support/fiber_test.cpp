#include <gtest/gtest.h>
#include <wayward/support/fiber.hpp>

namespace {
  using namespace wayward;

  TEST(Fiber, returning_from_fiber_resumes_caller) {
    bool ran = false;
    Fiber f { [&]() { ran = true; } };
    f();
    EXPECT_EQ(true, ran);
  }
}

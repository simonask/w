#include <gtest/gtest.h>
#include <wayward/support/fiber.hpp>

namespace {
  using namespace wayward;

  TEST(Fiber, returning_from_fiber_resumes_caller) {
    bool ran = false;
    auto f = fiber::create([&]() {
      ran = true;
    });
    fiber::resume(std::move(f));
    EXPECT_EQ(true, ran);
  }

  TEST(Fiber, calls_other_fiber) {
    FiberPtr main_fiber = fiber::current();
    FiberPtr b;
    int n = 0;
    auto a = fiber::create([&]() {
      while (n < 5) {
        fiber::resume(b);
      }
      fiber::resume(main_fiber);
    });
    b = fiber::create([&]() {
      while (true) {
        n += 1;
        fiber::resume(a);
      }
    });

    fiber::resume(a);
    EXPECT_EQ(5, n);
  }

  TEST(Fiber, terminates_gracefully) {
    auto main_fiber = fiber::current();
    int number = 0;

    auto f = fiber::create([&]() {
      struct Foo {
        int& n;
        Foo(int& n) : n(n) {}
        ~Foo() {
          n = 123;
        }
      };

      Foo foo { number };
      number = 789;
      fiber::resume(main_fiber);
      number = 456;
    });

    fiber::resume(f);
    EXPECT_EQ(789, number);
    fiber::terminate(f);
    EXPECT_EQ(123, number);
  }
}

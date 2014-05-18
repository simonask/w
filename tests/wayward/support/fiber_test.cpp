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

  TEST(Fiber, calls_other_fiber) {
    std::unique_ptr<Fiber> b;
    int n = 0;
    std::unique_ptr<Fiber> a (new Fiber { [&]() {
      while (n < 5) {
        b->resume();
      }
    }});
    b = std::unique_ptr<Fiber>(new Fiber { [&]() {
      while (true) {
        n += 1;
        a->resume();
      }
    }});

    a->resume();
    EXPECT_EQ(5, n);
  }

  TEST(Fiber, terminates_gracefully) {
    Fiber& main_fiber = Fiber::current();
    int number = 0;

    Fiber f { [&]() {
      struct Foo {
        int& n;
        Foo(int& n) : n(n) {}
        ~Foo() {
          n = 123;
        }
      };

      Foo foo { number };
      number = 789;
      main_fiber.resume();
      number = 456;
    }};

    f();
    EXPECT_EQ(789, number);
    f.terminate();
    EXPECT_EQ(123, number);
  }
}

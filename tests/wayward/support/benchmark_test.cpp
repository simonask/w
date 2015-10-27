#include <gtest/gtest.h>
#include <wayward/support/benchmark.hpp>

namespace wayward {
  void PrintTo(const wayward::DateTime& dt, std::ostream* os) {
    *os << dt.iso8601();
  }

  template <typename T>
  void PrintTo(DateTimeDuration<T> duration, std::ostream* os) {
    *os << duration.repr_.count() << " " << GetTimeUnitName<DateTimeDuration<T>>::Value;
  }
}

namespace {
  using wayward::DateTime;
  using wayward::Benchmark;
  using wayward::BenchmarkScope;
  using namespace wayward::units;

  TEST(Benchmark, measures_kernel_time) {
    Benchmark bm;
    ::usleep(100);
    auto t = bm.finish();
    EXPECT_GE(t, 100_us);
    EXPECT_NE(t, 0_us);
  }

  TEST(Benchmark, measures_userspace_time) {
    Benchmark bm;
    auto begin = DateTime::now();
    while (DateTime::now() - begin < 100_us) {
      // Do nothing, but try to avoid having the loop optimized out.
      ::usleep(0);
    }
    auto t = bm.finish();
    EXPECT_GE(t, 100_us);
  }

  TEST(BenchmarkScope, creates_a_subscope) {
    Benchmark bm;
    ::usleep(50);
    {
      BenchmarkScope BS { bm, "Subscope" };
      ::usleep(50);
    }
    auto total = bm.finish();
    EXPECT_GE(total, 100_us);
    auto scoped = bm.scopes()["Subscope"];
    EXPECT_GE(scoped, 50_us);
    EXPECT_NE(scoped, 0_us);
  }

  TEST(BenchmarkScope, accumulates_subscopes_in_depth) {
    Benchmark bm;
    {
      BenchmarkScope scope1 { bm, "Subscope" };
      ::usleep(50);
      {
        BenchmarkScope scope2 { bm, "Subscope" };
        ::usleep(50);
      }
    }
    auto total = bm.finish();
    EXPECT_EQ(1, bm.scopes().size());
    EXPECT_GE(bm.scopes()["Subscope"], 100_us);
  }
}

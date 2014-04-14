#include <gtest/gtest.h>
#include <wayward/support/datetime.hpp>
#include <wayward/testing/time_machine.hpp>

namespace {
  using wayward::DateTime;
  using wayward::DateTimeDuration;

  TEST(DateTime, instantiates_now) {
    DateTime now = DateTime::now();
  }

  TEST(DateTime, at_creates_a_date_at_nanosecond_precision) {
    auto date = DateTime::at(2012, 3, 21, 12, 21, 43, 100, 200, 300);
    EXPECT_EQ(2012, date.year());
    EXPECT_EQ(3, date.month());
    EXPECT_EQ(21, date.day());
    EXPECT_EQ(12, date.hour());
    EXPECT_EQ(21, date.minute());
    EXPECT_EQ(43, date.second());
    EXPECT_EQ(100, date.millisecond());
    EXPECT_EQ(200, date.microsecond());
    EXPECT_EQ(300, date.nanosecond());
  }
}

#include <gtest/gtest.h>
#include <wayward/support/datetime.hpp>
#include <wayward/testing/time_machine.hpp>

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
  using wayward::Timezone;
  using wayward::DateTimeDuration;
  using wayward::DateTimeInterval;
  using wayward::Nanoseconds;
  using wayward::Microseconds;
  using wayward::Milliseconds;
  using wayward::Seconds;
  using wayward::Minutes;
  using wayward::Hours;
  using wayward::Days;
  using wayward::Weeks;
  using wayward::Months;
  using wayward::Years;
  using namespace wayward::units;

  TEST(DateTime, instantiates_now) {
    DateTime now = DateTime::now();
  }

  TEST(DateTime, at_creates_a_date_at_nanosecond_precision) {
    auto date = DateTime::at(2012, 3, 21, 12, 21, 43, 100, 200, 300);
    EXPECT_EQ(2012_years, date.year());
    EXPECT_EQ(3_months, date.month());
    EXPECT_EQ(21_days, date.day());
    EXPECT_EQ(12_hours, date.hour());
    EXPECT_EQ(21_minutes, date.minute());
    EXPECT_EQ(43_seconds, date.second());
    EXPECT_EQ(100_milliseconds, date.millisecond());
    EXPECT_EQ(200_microseconds, date.microsecond());
    EXPECT_EQ(300_nanoseconds, date.nanosecond());
  }

  TEST(DateTime, strftime_formats_iso8601) {
    auto date = DateTime::at(2012, 3, 21, 12, 21, 43);
    auto formatted = date.strftime("%Y-%m-%d %H:%M:%S");
    EXPECT_EQ("2012-03-21 12:21:43", formatted);
  }

  TEST(DateTime, adds_nanoseconds) {
    auto date = DateTime::at(2012, 3, 21, 12, 21, 43, 100, 200, 300);
    auto x = date + Nanoseconds{10};
    auto expected = DateTime::at(2012, 3, 21, 12, 21, 43, 100, 200, 310);
    EXPECT_EQ(x, expected);
  }

  TEST(DateTime, overflowing_nanoseconds_become_microseconds) {
    auto date =     DateTime::at(2012, 3, 21, 12, 21, 43, 100, 200, 1300);
    auto expected = DateTime::at(2012, 3, 21, 12, 21, 43, 100, 201,  300);
    EXPECT_EQ(date, expected);
  }

  TEST(DateTime, adds_microseconds) {
    auto date = DateTime::at(2012, 3, 21, 12, 21, 43, 100, 200, 300);
    auto x = date + Microseconds{10};
    auto expected = DateTime::at(2012, 3, 21, 12, 21, 43, 100, 210, 300);
    EXPECT_EQ(x, expected);
  }

  TEST(DateTime, overflowing_microseconds_become_milliseconds) {
    auto date =     DateTime::at(2012, 3, 21, 12, 21, 43, 100, 1200, 300);
    auto expected = DateTime::at(2012, 3, 21, 12, 21, 43, 101,  200, 300);
    EXPECT_EQ(date, expected);
  }

  TEST(DateTime, adds_milliseconds) {
    auto date = DateTime::at(2012, 3, 21, 12, 21, 43, 100, 200, 300);
    auto x = date + Milliseconds{10};
    auto expected = DateTime::at(2012, 3, 21, 12, 21, 43, 110, 200, 300);
    EXPECT_EQ(x, expected);
  }

  TEST(DateTime, overflowing_milliseconds_become_seconds) {
    auto date =     DateTime::at(2012, 3, 21, 12, 21, 43, 1100, 200, 300);
    auto expected = DateTime::at(2012, 3, 21, 12, 21, 44,  100, 200, 300);
    EXPECT_EQ(date, expected);
  }

  TEST(DateTime, overflowing_seconds_become_minutes) {
    auto date =     DateTime::at(2012, 3, 21, 12, 21, 61, 100, 200, 300);
    auto expected = DateTime::at(2012, 3, 21, 12, 22,  1, 100, 200, 300);
    EXPECT_EQ(date, expected);
  }

  TEST(DateTime, overflowing_minutes_become_hours) {
    auto date =     DateTime::at(2012, 3, 21, 12, 61, 43, 100, 200, 300);
    auto expected = DateTime::at(2012, 3, 21, 13, 1,  43, 100, 200, 300);
    EXPECT_EQ(date, expected);
  }

  TEST(DateTime, overflowing_hours_become_days) {
    auto date =     DateTime::at(2012, 3, 21, 25, 21, 43, 100, 200, 300);
    auto expected = DateTime::at(2012, 3, 22,  1, 21, 43, 100, 200, 300);
    EXPECT_EQ(date, expected);
  }

  TEST(DateTime, overflowing_days_become_months) {
    auto date =     DateTime::at(2012, 3, 32, 12, 21, 43, 100, 200, 300);
    auto expected = DateTime::at(2012, 4, 1,  12, 21, 43, 100, 200, 300);
    EXPECT_EQ(date.month(), 4);
    EXPECT_EQ(date, expected);
  }

  TEST(DateTime, overflowing_days_become_months_in_regular_february) {
    auto date =     DateTime::at(2011, 2, 29, 12, 21, 43, 100, 200, 300);
    auto expected = DateTime::at(2011, 3, 1,  12, 21, 43, 100, 200, 300);
    EXPECT_EQ(date, expected);
  }

  TEST(DateTime, overflowing_days_become_months_in_leap_year_february) {
    auto date =     DateTime::at(2012, 2, 30, 12, 21, 43, 100, 200, 300);
    auto expected = DateTime::at(2012, 3, 1,  12, 21, 43, 100, 200, 300);
    EXPECT_EQ(date, expected);
  }

  TEST(DateTime, overflowing_nanoseconds_become_years) {
    auto date =     DateTime::at(2012, 12, 31, 23, 59, 59, 999, 999, 1000);
    auto expected = DateTime::at(2013,  1,  1,  0,  0,  0,   0,   0,    0);
    EXPECT_EQ(date, expected);
  }

  TEST(DateTime, add_integer_month_to_get_same_day_of_month) {
    auto date = DateTime::at(2012, 3, 21, 12, 21, 43, 100, 200, 300);
    auto x = date + Months{1};
    auto expected = DateTime::at(2012, 4, 21, 12, 21, 43, 100, 200, 300);
    EXPECT_EQ(x, expected);
  }

  TEST(DateTime, adding_integer_month_gets_ultimo_in_leap_year) {
    auto date = DateTime::at(2012, 1, 31, 12, 21, 43, 100, 200, 300);
    auto x = date + Months{1};
    auto expected = DateTime::at(2012, 2, 29, 12, 21, 43, 100, 200, 300);
    EXPECT_EQ(x, expected);
  }

  TEST(DateTime, adding_one_year_to_leap_date_give_ultimo_february) {
    auto date = DateTime::at(2012, 2, 29, 0, 0, 0);
    auto x = date + Years{1};
    auto expected = DateTime::at(2013, 2, 28, 0, 0, 0);
    EXPECT_EQ(x, expected);
  }

  TEST(DateTime, DISABLED_treats_1900_as_common_year) {
    // TODO: Disabled because timegm is acting inconsistently with dates before 1970.
    // Solution: Do our own fallback version of timegm that gets called for pre-1970 dates.
    auto date = DateTime::at(1900, 5, 32, 0, 0, 0);
    EXPECT_EQ(date.month(), 6);
  }

  TEST(DateTime, treats_2000_as_leap_year) {
    auto date = DateTime::at(2000, 2, 29, 0, 0, 0);
    EXPECT_EQ(date.month(), 2_months);
    auto date2 = DateTime::at(2000, 2, 30, 0, 0, 0);
    EXPECT_EQ(date2.month(), 3_months);
  }

  TEST(DateTime, treats_2001_as_common_year) {
    auto date = DateTime::at(2001, 2, 29, 0, 0, 0);
    EXPECT_EQ(date.month(), 3_months);
  }

  TEST(DateTime, treats_1980_as_leap_year) {
    auto date = DateTime::at(1980, 2, 29, 0, 0, 0);
    EXPECT_EQ(date.month(), 2_months);
  }

  TEST(DateTimeInterval, adds_seconds_and_minutes) {
    DateTimeInterval seconds { 30_seconds };
    DateTimeInterval minutes { 1_minute };
    auto x = minutes + seconds;
    auto expected = 90_seconds;
    EXPECT_EQ(x, expected);
  }

  TEST(DateTime, DISABLED_strptime_regression_1) {
    // This is disabled because strftime/strptime relate to the current time zone, but
    // strptime without time zone seems to assume that the input is UTC, which makes it
    // impossible to test with string comparison.

    std::string repr = "2014-06-15 12:07:00";
    auto m = DateTime::strptime(repr, "%Y-%m-%d %T");
    EXPECT_EQ(true, (bool)m);
    EXPECT_EQ(2014_years, m->year());
    EXPECT_EQ(6_months, m->month());
    EXPECT_EQ(15_days, m->day());
    EXPECT_EQ(12_hours, m->hour());
    EXPECT_EQ(7_minutes, m->minute());
    EXPECT_EQ(0_seconds, m->second());
    printf("TIME WITH TZ: %s\n", m->strftime("%Y-%m-%d %T %Z").c_str());
    EXPECT_EQ(repr, m->strftime("%Y-%m-%d %T"));
  }
}

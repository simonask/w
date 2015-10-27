#pragma once
#ifndef WAYWARD_SUPPORT_DATETIME_DATETIME_HPP_INCLUDED
#define WAYWARD_SUPPORT_DATETIME_DATETIME_HPP_INCLUDED

#include <wayward/support/datetime/clock.hpp>
#include <wayward/support/datetime/duration_units.hpp>
#include <wayward/support/datetime/interval.hpp>
#include <wayward/support/datetime/timezone.hpp>

#include <wayward/support/maybe.hpp>

#include <chrono>
#include <string>

namespace wayward {
  template <typename T> struct DateTimeArithmetic;

  /*
    DateTime represents a UTC point in time, with nanosecond precision.
  */
  struct DateTime {
    using Clock = std::chrono::system_clock;
    using Repr = std::chrono::time_point<Clock, std::chrono::nanoseconds>;

    DateTime() {}
    explicit DateTime(Repr repr) : repr_(repr) {}
    DateTime(const DateTime&) = default;
    Repr& r() { return repr_; }
    DateTime& operator=(const DateTime&) = default;

    template <typename T>
    DateTime operator+(DateTimeDuration<T> duration) {
      DateTime copy = *this;
      copy += duration;
      return copy;
    }
    template <typename T>
    DateTime operator-(DateTimeDuration<T> duration) {
      DateTime copy = *this;
      copy -= duration;
      return copy;
    }
    template <typename T>
    DateTime& operator+=(DateTimeDuration<T> duration) {
      DateTimeArithmetic<DateTimeDuration<T>>::adjust(*this, duration);
      return *this;
    }
    template <typename T>
    DateTime& operator-=(DateTimeDuration<T> duration) {
      DateTimeArithmetic<DateTimeDuration<T>>::adjust(*this, -duration);
    }
    DateTime operator+(const DateTimeInterval& interval) const;
    DateTime operator-(const DateTimeInterval& interval) const;
    DateTime& operator+=(const DateTimeInterval& interval);
    DateTime& operator-=(const DateTimeInterval& interval);

    DateTimeInterval operator-(DateTime other) const;

    bool operator==(const DateTime& other) const { return repr_ == other.repr_; }
    bool operator!=(const DateTime& other) const { return repr_ != other.repr_; }
    bool operator<(const DateTime& other)  const { return repr_ <  other.repr_; }
    bool operator>(const DateTime& other)  const { return repr_ >  other.repr_; }
    bool operator<=(const DateTime& other) const { return repr_ <= other.repr_; }
    bool operator>=(const DateTime& other) const { return repr_ >= other.repr_; }

    static DateTime now() { return clock().now(); }

    Years year() const;
    Months month() const;
    Days day() const;
    Hours hour() const;
    Minutes minute() const;
    Seconds second() const;
    Milliseconds millisecond() const;
    Microseconds microsecond() const;
    Nanoseconds nanosecond() const;

    Seconds unix_timestamp() const;
    std::string strftime(const std::string& format) const;
    std::string iso8601() const;

    struct CalendarValues {
      Years year;
      Months month;
      Days day;
      Hours hour;
      Minutes minute;
      Seconds second;
      Milliseconds millisecond;
      Microseconds microsecond;
      Nanoseconds nanosecond;
      Timezone timezone = Timezone::UTC;
    };

    CalendarValues as_calendar_values() const;

    /*
      This creates a DateTime for the specified time. It automatically adjusts out-of-range and negative values, such that:
      at(2011, 13, 1, 0, 0, 0) == at(2012, 1, 1, 0, 0, 0)
      at(2010, 2, 29, 0, 0, 0) == at(2010, 3, 1, 0, 0, 0) // 2010 was not a leap year
      at(2012, 3, 4, 30, 0, 0) == at(2010, 3, 5, 6, 0, 0)
      at(2014, 3, 4, -6, 0, 0) == at(2010, 3, 3, 18, 0, 0)

      NOTE: Months/days start at 1.
    */
    static DateTime at(Years year, Months month, Days d, Hours h, Minutes m, Seconds s, Milliseconds ms = 0, Microseconds us = 0, Nanoseconds ns = 0);
    static DateTime at(Timezone tz, Years year, Months month, Days d, Hours h, Minutes m, Seconds s, Milliseconds ms = 0, Microseconds us = 0, Nanoseconds ns = 0);
    static DateTime at(const CalendarValues& calendar_values);

    static Maybe<DateTime> strptime(const std::string& input, const std::string& format);

    Repr repr_;
  };

  template <>
  struct DateTimeArithmetic<Years> {
    static void adjust(DateTime& t, Years by);
  };

  template <>
  struct DateTimeArithmetic<Months> {
    static void adjust(DateTime& t, Months by);
  };

  template <>
  struct DateTimeArithmetic<DateTimeInterval> {
    static bool is_months(const DateTimeInterval& interval);
    static bool is_years(const DateTimeInterval& interval);

    static void adjust(DateTime& t, const DateTimeInterval& interval);
  };

  /*
    Fallback for "simple" intervals that are always the same length in seconds
    (i.e. weeks and below).
  */
  template <typename T>
  struct DateTimeArithmetic<DateTimeDuration<T>> {
    static void adjust(DateTime& time_point, DateTimeDuration<T> by) {
      time_point.repr_ += by.repr_;
    }
  };
}

#endif // WAYWARD_SUPPORT_DATETIME_DATETIME_HPP_INCLUDED

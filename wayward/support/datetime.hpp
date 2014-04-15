#pragma once
#ifndef WAYWARD_SUPPORT_DATETIME_HPP_INCLUDED
#define WAYWARD_SUPPORT_DATETIME_HPP_INCLUDED

#include <chrono>
#include <string>

namespace wayward {
  template <typename IntervalType>
  struct DateTimeDuration {
    using Repr = typename IntervalType::rep;
    using RatioToSeconds = typename IntervalType::period;
    using Self = DateTimeDuration<IntervalType>;
    explicit DateTimeDuration(Repr r) : repr_(r) {}
    DateTimeDuration(IntervalType repr) : repr_(repr) {}
    operator IntervalType() const { return repr_; }
    Self& operator=(const Self&) = default;

    template <typename T>
    auto operator+(DateTimeDuration<T> other) -> DateTimeDuration<decltype(std::declval<IntervalType>() + other.repr_)> {
      return repr_ + other.repr_;
    }
    template <typename T>
    auto operator-(DateTimeDuration<T> other) -> DateTimeDuration<decltype(std::declval<IntervalType>() - other.repr_)> {
      return repr_ - other.repr_;
    }
    template <typename T>
    auto operator*(T scalar) -> DateTimeDuration<decltype(std::declval<IntervalType>() * scalar)> {
      return repr_ * scalar;
    }
    template <typename T>
    auto operator/(DateTimeDuration<T> other) -> decltype(std::declval<IntervalType>() / other.repr_) {
      return repr_ / other.repr_;
    }
    template <typename T>
    auto operator/(T scalar) -> DateTimeDuration<decltype(std::declval<IntervalType>() / scalar)> {
      return repr_ / scalar;
    }
    template <typename T>
    auto operator%(DateTimeDuration<T> other) -> DateTimeDuration<decltype(std::declval<IntervalType>() * other.repr_)> {
      return repr_ % other.repr_;
    }
    template <typename T>
    auto operator%(T scalar) -> decltype(std::declval<IntervalType>() % scalar) {
      return repr_ % scalar;
    }

    Self& operator+=(Self other) {
      repr_ += other.repr_;
      return *this;
    }

    Self& operator-=(Self other) {
      repr_ -= other.repr_;
      return *this;
    }

    Self& operator*=(Repr scalar) {
      repr_ *= scalar;
      return *this;
    }

    Self& operator/=(Repr scalar) {
      repr_ /= scalar;
      return *this;
    }

    Self& operator%=(Self other) {
      repr_ %= other.repr_;
      return *this;
    }

    IntervalType repr_;
  };

  using Years        = DateTimeDuration<std::chrono::duration<int64_t, std::ratio<365*24*60*60>>>;
  using Months       = DateTimeDuration<std::chrono::duration<int64_t, std::ratio<30*24*60*60>>>;
  using Weeks        = DateTimeDuration<std::chrono::duration<int64_t, std::ratio<7*24*60*60>>>;
  using Days         = DateTimeDuration<std::chrono::duration<int64_t, std::ratio<24*60*60>>>;
  using Hours        = DateTimeDuration<std::chrono::hours>;
  using Minutes      = DateTimeDuration<std::chrono::minutes>;
  using Seconds      = DateTimeDuration<std::chrono::seconds>;
  using Milliseconds = DateTimeDuration<std::chrono::milliseconds>;
  using Microseconds = DateTimeDuration<std::chrono::microseconds>;
  using Nanoseconds  = DateTimeDuration<std::chrono::nanoseconds>;

  inline Years        operator""_years(unsigned long long years)     { return Years{(int64_t)years}; }
  inline Months       operator""_months(unsigned long long months)   { return Months{(int64_t)months}; }
  inline Weeks        operator""_weeks(unsigned long long weeks)     { return Weeks{(int64_t)weeks}; }
  inline Days         operator""_days(unsigned long long days)       { return Days{(int64_t)days}; }
  inline Hours        operator""_hours(unsigned long long hours)     { return Hours{(int64_t)hours}; }
  inline Minutes      operator""_minutes(unsigned long long minutes) { return Minutes{(int64_t)minutes}; }
  inline Seconds      operator""_seconds(unsigned long long seconds) { return Seconds{(int64_t)seconds}; }
  inline Milliseconds operator""_milliseconds(unsigned long long ms) { return Milliseconds{(int64_t)ms}; }
  inline Milliseconds operator""_ms(unsigned long long ms)           { return Milliseconds{(int64_t)ms}; }
  inline Microseconds operator""_microseconds(unsigned long long us) { return Microseconds{(int64_t)us}; }
  inline Microseconds operator""_us(unsigned long long us)           { return Microseconds{(int64_t)us}; }
  inline Nanoseconds  operator""_nanoseconds(unsigned long long ns)  { return Nanoseconds{(int64_t)ns}; }
  inline Nanoseconds  operator""_ns(unsigned long long ns)           { return Nanoseconds{(int64_t)ns}; }

  struct DateTimeInterval {
    DateTimeInterval() : value_(0.0), num_(1), denom_(1) {}
    template <typename T>
    DateTimeInterval(DateTimeDuration<T> duration);
    DateTimeInterval(const DateTimeInterval&) = default;
    DateTimeInterval& operator=(const DateTimeInterval&) = default;

    Years years() const;
    Months months() const;
    Weeks weeks() const;
    Days days() const;
    Hours hours() const;
    Minutes minutes() const;
    Seconds seconds() const;
    Milliseconds milliseconds() const;
    Microseconds microseconds() const;
    Nanoseconds nanoseconds() const;

    DateTimeInterval operator-() const { DateTimeInterval copy = *this; copy.value_ = -copy.value_; return copy; }
    DateTimeInterval& operator+=(const DateTimeInterval& other);
    DateTimeInterval& operator-=(const DateTimeInterval& other);
    DateTimeInterval& operator*=(double scalar);
    DateTimeInterval& operator/=(double scalar);
    double operator/(const DateTimeInterval& other);
    double operator%(const DateTimeInterval& other);

    double value() const { return value_; }
    uint64_t numerator() const { return num_; }
    uint64_t denominator() const { return denom_; }
  private:
    double value_;
    uint64_t num_;
    uint64_t denom_;
  };

  struct DateTime;

  struct IClock {
    virtual ~IClock() {}
    virtual DateTime now() const = 0;
  };

  IClock& clock();

  template <typename T> struct DateTimeArithmetic;

  struct DateTime {
    using Clock = std::chrono::system_clock;
    using Repr = std::chrono::time_point<Clock, std::chrono::nanoseconds>;

    DateTime() {}
    DateTime(Repr repr) : repr_(std::move(repr)) {}
    operator Repr() const { return repr_; }
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

    DateTimeInterval operator-(DateTime other);

    bool operator==(const DateTime&) const;
    bool operator!=(const DateTime&) const;
    bool operator<(const DateTime&) const;
    bool operator>(const DateTime&) const;
    bool operator<=(const DateTime&) const;
    bool operator>=(const DateTime&) const;

    static DateTime now() { return clock().now(); }

    int64_t year() const;
    int64_t month() const;
    int64_t day() const;
    int64_t hour() const;
    int64_t minute() const;
    int64_t second() const;
    int64_t millisecond() const;
    int64_t microsecond() const;
    int64_t nanosecond() const;

    Seconds unix_timestamp() const;
    std::string strftime(const std::string& format) const;
    std::string iso8601() const;

    struct CalendarValues {
      int64_t year;
      int64_t month;
      int64_t day;
      int64_t hour;
      int64_t minute;
      int64_t second;
      int64_t millisecond;
      int64_t microsecond;
      int64_t nanosecond;
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
    static DateTime at(int64_t year, int32_t month, int64_t d, int64_t h, int64_t m, int64_t s, int64_t ms = 0, int64_t us = 0, int64_t ns = 0);
    static DateTime at(const CalendarValues& calendar_values);

    Repr repr_;
  };

  struct SystemClock : IClock {
    DateTime now() const final;
    static SystemClock& get();
  private:
    SystemClock() {}
  };

  template <>
  struct DateTimeArithmetic<Years> {
    static void adjust(DateTime& t, Years by) {
      DateTime::CalendarValues cal = t.as_calendar_values();
      cal.year += by.repr_.count();
      t = DateTime::at(cal);
    }
  };

  template <>
  struct DateTimeArithmetic<Months> {
    static void adjust(DateTime& t, Months by) {
      DateTime::CalendarValues cal = t.as_calendar_values();
      cal.month += by.repr_.count();
      t = DateTime::at(cal);
    }
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

#endif // WAYWARD_SUPPORT_DATETIME_HPP_INCLUDED

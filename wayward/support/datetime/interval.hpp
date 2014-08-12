#pragma once
#ifndef WAYWARD_SUPPORT_DATETIME_INTERVAL_HPP_INCLUDED
#define WAYWARD_SUPPORT_DATETIME_INTERVAL_HPP_INCLUDED

#include <wayward/support/datetime/duration_units.hpp>
#include <ostream>

struct timeval;

namespace wayward {
  struct DateTimeInterval {
    DateTimeInterval() {}
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
    DateTimeInterval operator+(const DateTimeInterval&) const;
    DateTimeInterval operator-(const DateTimeInterval&) const;
    DateTimeInterval operator*(double scalar) const;
    DateTimeInterval operator/(double scalar) const;
    DateTimeInterval& operator+=(const DateTimeInterval& other);
    DateTimeInterval& operator-=(const DateTimeInterval& other);
    DateTimeInterval& operator*=(double scalar);
    DateTimeInterval& operator/=(double scalar);
    double operator/(const DateTimeInterval& other);
    double operator%(const DateTimeInterval& other);

    double value() const { return value_; }
    uint32_t numerator() const { return num_; }
    uint32_t denominator() const { return denom_; }

    struct timeval to_timeval() const;
  private:
    double value_ = 0.0;
    uint32_t num_ = 1;
    uint32_t denom_ = 1;
  };

  template <typename T>
  DateTimeInterval::DateTimeInterval(DateTimeDuration<T> duration)
  : value_(duration.repr_.count())
  , num_(T::period::num)
  , denom_(T::period::den)
  {}

  std::ostream& operator<<(std::ostream&, const DateTimeInterval&);

  bool operator==(const DateTimeInterval& a, const DateTimeInterval& b);
  bool operator!=(const DateTimeInterval& a, const DateTimeInterval& b);
  bool operator<(const DateTimeInterval& a, const DateTimeInterval& b);
  bool operator>(const DateTimeInterval& a, const DateTimeInterval& b);
  bool operator<=(const DateTimeInterval& a, const DateTimeInterval& b);
  bool operator>=(const DateTimeInterval& a, const DateTimeInterval& b);
}

#endif // WAYWARD_SUPPORT_DATETIME_INTERVAL_HPP_INCLUDED

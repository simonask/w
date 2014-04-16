#pragma once
#ifndef WAYWARD_SUPPORT_DATETIME_DURATION_UNITS_HPP_INCLUDED
#define WAYWARD_SUPPORT_DATETIME_DURATION_UNITS_HPP_INCLUDED

#include <chrono>

namespace wayward {
  /*
    DateTimeDuration represents an integer duration, i.e., 'minutes' represents a whole number of
    units, and in order to represent 1:30 minutes, you have to say '90 seconds'.
    This is in contrast to DateTimeInterval, which has floating ratios.
  */
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
}

#endif // WAYWARD_SUPPORT_DATETIME_DURATION_UNITS_HPP_INCLUDED

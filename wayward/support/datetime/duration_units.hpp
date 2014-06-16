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
    DateTimeDuration(Repr r = 0) : repr_(r) {}
    DateTimeDuration(IntervalType interval) : repr_(interval) {}

    DateTimeDuration(const DateTimeDuration<IntervalType>&) = default;

    template <typename OtherIntervalType>
    DateTimeDuration(DateTimeDuration<OtherIntervalType> other) {
      using OtherRatio = typename DateTimeDuration<OtherIntervalType>::RatioToSeconds;
      static_assert(OtherRatio::den <= RatioToSeconds::den, "Converting to duration with longer denominator would lose precision.");
      const auto mul = OtherRatio::num * RatioToSeconds::den;
      const auto divide = OtherRatio::den * RatioToSeconds::num;
      repr_ = IntervalType{other.count() * mul / divide};
    }

    bool operator==(const DateTimeDuration<IntervalType>& other) const { return repr_ == other.repr_; }
    bool operator!=(const DateTimeDuration<IntervalType>& other) const { return repr_ != other.repr_; }
    bool operator<(const DateTimeDuration<IntervalType>& other) const { return repr_ < other.repr_; }
    bool operator>(const DateTimeDuration<IntervalType>& other) const { return repr_ > other.repr_; }
    bool operator<=(const DateTimeDuration<IntervalType>& other) const { return repr_ <= other.repr_; }
    bool operator>=(const DateTimeDuration<IntervalType>& other) const { return repr_ >= other.repr_; }

    Repr count() const { return repr_.count(); }

    operator IntervalType() const { return repr_; }
    Self& operator=(const Self&) = default;

    template <typename T>
    auto operator+(DateTimeDuration<T> other) const -> DateTimeDuration<decltype(std::declval<IntervalType>() + other.repr_)> {
      return repr_ + other.repr_;
    }
    template <typename T>
    auto operator-(DateTimeDuration<T> other) const -> DateTimeDuration<decltype(std::declval<IntervalType>() - other.repr_)> {
      return repr_ - other.repr_;
    }
    template <typename T>
    auto operator*(T scalar) const -> DateTimeDuration<decltype(std::declval<IntervalType>() * scalar)> {
      return repr_ * scalar;
    }
    template <typename T>
    auto operator/(DateTimeDuration<T> other) const -> decltype(std::declval<IntervalType>() / other.repr_) {
      return repr_ / other.repr_;
    }
    template <typename T>
    auto operator/(T scalar) const -> DateTimeDuration<decltype(std::declval<IntervalType>() / scalar)> {
      return repr_ / scalar;
    }
    template <typename T>
    auto operator%(DateTimeDuration<T> other) const -> DateTimeDuration<decltype(std::declval<IntervalType>() * other.repr_)> {
      return repr_ % other.repr_;
    }
    template <typename T>
    auto operator%(T scalar) const -> DateTimeDuration<decltype(std::declval<IntervalType>() % scalar)> {
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

    Self operator-() const {
      return Self{-repr_};
    }

    IntervalType repr_;
  };

  template <typename T, typename IntervalType>
  typename std::enable_if<std::is_arithmetic<T>::value, DateTimeDuration<IntervalType>>::type
  operator*(T scalar, DateTimeDuration<IntervalType> duration) {
    return duration * scalar;
  }

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

  template <typename T> struct GetTimeUnitName;
  template <> struct GetTimeUnitName<Years> { static constexpr const char Value[] = "years"; };
  template <> struct GetTimeUnitName<Months> { static constexpr const char Value[] = "months"; };
  template <> struct GetTimeUnitName<Weeks> { static constexpr const char Value[] = "weeks"; };
  template <> struct GetTimeUnitName<Days> { static constexpr const char Value[] = "days"; };
  template <> struct GetTimeUnitName<Hours> { static constexpr const char Value[] = "hours"; };
  template <> struct GetTimeUnitName<Minutes> { static constexpr const char Value[] = "minutes"; };
  template <> struct GetTimeUnitName<Seconds> { static constexpr const char Value[] = "seconds"; };
  template <> struct GetTimeUnitName<Milliseconds> { static constexpr const char Value[] = "milliseconds"; };
  template <> struct GetTimeUnitName<Microseconds> { static constexpr const char Value[] = "microseconds"; };
  template <> struct GetTimeUnitName<Nanoseconds> { static constexpr const char Value[] = "nanoseconds"; };

  namespace units {
    inline Years        operator"" _years(unsigned long long years)     { return Years{(Years::Repr)years}; }
    inline Months       operator"" _months(unsigned long long months)   { return Months{(Months::Repr)months}; }
    inline Weeks        operator"" _weeks(unsigned long long weeks)     { return Weeks{(Weeks::Repr)weeks}; }
    inline Days         operator"" _days(unsigned long long days)       { return Days{(Days::Repr)days}; }
    inline Hours        operator"" _hours(unsigned long long hours)     { return Hours{(Hours::Repr)hours}; }
    inline Minutes      operator"" _minutes(unsigned long long minutes) { return Minutes{(Minutes::Repr)minutes}; }
    inline Seconds      operator"" _seconds(unsigned long long seconds) { return Seconds{(Seconds::Repr)seconds}; }
    inline Milliseconds operator"" _milliseconds(unsigned long long ms) { return Milliseconds{(Milliseconds::Repr)ms}; }
    inline Milliseconds operator"" _ms(unsigned long long ms)           { return Milliseconds{(Milliseconds::Repr)ms}; }
    inline Microseconds operator"" _microseconds(unsigned long long us) { return Microseconds{(Microseconds::Repr)us}; }
    inline Microseconds operator"" _us(unsigned long long us)           { return Microseconds{(Microseconds::Repr)us}; }
    inline Nanoseconds  operator"" _nanoseconds(unsigned long long ns)  { return Nanoseconds{(Nanoseconds::Repr)ns}; }
    inline Nanoseconds  operator"" _ns(unsigned long long ns)           { return Nanoseconds{(Nanoseconds::Repr)ns}; }

    // Singular versions:
    inline Years        operator"" _year(unsigned long long years)      { return Years{(Years::Repr)years}; }
    inline Months       operator"" _month(unsigned long long months)    { return Months{(Months::Repr)months}; }
    inline Weeks        operator"" _week(unsigned long long weeks)      { return Weeks{(Weeks::Repr)weeks}; }
    inline Days         operator"" _day(unsigned long long days)        { return Days{(Days::Repr)days}; }
    inline Hours        operator"" _hour(unsigned long long hours)      { return Hours{(Hours::Repr)hours}; }
    inline Minutes      operator"" _minute(unsigned long long minutes)  { return Minutes{(Minutes::Repr)minutes}; }
    inline Seconds      operator"" _second(unsigned long long seconds)  { return Seconds{(Seconds::Repr)seconds}; }
    inline Milliseconds operator"" _millisecond(unsigned long long ms)  { return Milliseconds{(Milliseconds::Repr)ms}; }
    inline Microseconds operator"" _microsecond(unsigned long long us)  { return Microseconds{(Microseconds::Repr)us}; }
    inline Nanoseconds  operator"" _nanosecond(unsigned long long ns)   { return Nanoseconds{(Nanoseconds::Repr)ns}; }
  }
}

#endif // WAYWARD_SUPPORT_DATETIME_DURATION_UNITS_HPP_INCLUDED

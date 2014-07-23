#include <wayward/support/datetime/interval.hpp>

#include <utility>

namespace wayward {
  Years DateTimeInterval::years() const {
    double seconds = value_ * num_ / denom_;
    using Ratio = Years::RatioToSeconds;
    return Years{(Years::Repr)(seconds * Ratio::den / Ratio::num)};
  }

  Months DateTimeInterval::months() const {
    double seconds = value_ * num_ / denom_;
    using Ratio = Months::RatioToSeconds;
    return Months{(Months::Repr)(seconds * Ratio::den / Ratio::num)};
  }

  Weeks DateTimeInterval::weeks() const {
    double seconds = value_ * num_ / denom_;
    using Ratio = Weeks::RatioToSeconds;
    return Weeks{(Weeks::Repr)(seconds * Ratio::den / Ratio::num)};
  }

  Days DateTimeInterval::days() const {
    double seconds = value_ * num_ / denom_;
    using Ratio = Days::RatioToSeconds;
    return Days{(Days::Repr)(seconds * Ratio::den / Ratio::num)};
  }

  Hours DateTimeInterval::hours() const {
    double seconds = value_ * num_ / denom_;
    using Ratio = Hours::RatioToSeconds;
    return Hours{(Hours::Repr)(seconds * Ratio::den / Ratio::num)};
  }

  Minutes DateTimeInterval::minutes() const {
    double seconds = value_ * num_ / denom_;
    using Ratio = Minutes::RatioToSeconds;
    return Minutes{(Minutes::Repr)(seconds * Ratio::den / Ratio::num)};
  }

  Seconds DateTimeInterval::seconds() const {
    double seconds = value_ * num_ / denom_;
    using Ratio = Seconds::RatioToSeconds;
    return Seconds{(Seconds::Repr)(seconds * Ratio::den / Ratio::num)};
  }

  Milliseconds DateTimeInterval::milliseconds() const {
    double seconds = value_ * num_ / denom_;
    using Ratio = Milliseconds::RatioToSeconds;
    return Milliseconds{(Milliseconds::Repr)(seconds * Ratio::den / Ratio::num)};
  }

  Microseconds DateTimeInterval::microseconds() const {
    double seconds = value_ * num_ / denom_;
    using Ratio = Microseconds::RatioToSeconds;
    return Microseconds{(Microseconds::Repr)(seconds * Ratio::den / Ratio::num)};
  }

  Nanoseconds DateTimeInterval::nanoseconds() const {
    double seconds = value_ * num_ / denom_;
    using Ratio = Nanoseconds::RatioToSeconds;
    return Nanoseconds{(Nanoseconds::Repr)(seconds * Ratio::num / Ratio::den)};
  }


  namespace {
    std::pair<double, double>
    common_unit(const DateTimeInterval& a, const DateTimeInterval& b, uint32_t& out_numerator, uint32_t& out_denominator) {
      double a_ratio = (double)a.numerator() / (double)a.denominator();
      double b_ratio = (double)b.numerator() / (double)b.denominator();
      if (a_ratio > b_ratio) {
        // A is a bigger-grained unit, so convert it to b's unit:
        double a_normalized = a.value() * a_ratio;
        double a_converted = a_normalized * b_ratio;
        out_numerator = b.numerator();
        out_denominator = b.denominator();
        return std::make_pair(a_converted, b.value());
      } else if (a_ratio < b_ratio) {
        // B is bigger-grained unit, so convert it to a's unit:
        double b_normalized = b.value() * b_ratio;
        double b_converted = b_normalized * a_ratio;
        out_numerator = a.numerator();
        out_denominator = a.denominator();
        return std::make_pair(a.value(), b_converted);
      } else {
        // Same ratio!
        return std::make_pair(a.value(), b.value());
      }
    }

    std::pair<double, double>
    common_unit(const DateTimeInterval& a, const DateTimeInterval& b) {
      uint32_t numerator, denominator;
      return common_unit(a, b, numerator, denominator);
    }
  }

  bool operator==(const DateTimeInterval& a, const DateTimeInterval& b) {
    auto common = common_unit(a, b);
    return common.first == common.second;
  }

  bool operator!=(const DateTimeInterval& a, const DateTimeInterval& b) {
    auto common = common_unit(a, b);
    return common.first != common.second;
  }

  bool operator<(const DateTimeInterval& a, const DateTimeInterval& b) {
    auto common = common_unit(a, b);
    return common.first < common.second;
  }

  bool operator>(const DateTimeInterval& a, const DateTimeInterval& b) {
    auto common = common_unit(a, b);
    return common.first > common.second;
  }

  bool operator<=(const DateTimeInterval& a, const DateTimeInterval& b) {
    auto common = common_unit(a, b);
    return common.first <= common.second;
  }

  bool operator>=(const DateTimeInterval& a, const DateTimeInterval& b) {
    auto common = common_unit(a, b);
    return common.first >= common.second;
  }

  DateTimeInterval DateTimeInterval::operator+(const DateTimeInterval& other) const {
    DateTimeInterval copy = *this;
    copy += other;
    return copy;
  }

  DateTimeInterval DateTimeInterval::operator-(const DateTimeInterval& other) const {
    DateTimeInterval copy = *this;
    copy -= other;
    return copy;
  }

  DateTimeInterval DateTimeInterval::operator*(double scalar) const {
    DateTimeInterval copy = *this;
    copy *= scalar;
    return copy;
  }

  DateTimeInterval DateTimeInterval::operator/(double scalar) const {
    DateTimeInterval copy = *this;
    copy /= scalar;
    return copy;
  }

  DateTimeInterval& DateTimeInterval::operator+=(const DateTimeInterval& other) {
    auto common = common_unit(*this, other, num_, denom_);
    value_ = common.first + common.second;
    return *this;
  }

  DateTimeInterval& DateTimeInterval::operator-=(const DateTimeInterval& other) {
    auto common = common_unit(*this, other, num_, denom_);
    value_ = common.first - common.second;
    return *this;
  }

  DateTimeInterval& DateTimeInterval::operator*=(double scalar) {
    value_ *= scalar;
    return *this;
  }

  DateTimeInterval& DateTimeInterval::operator/=(double scalar) {
    value_ /= scalar;
    return *this;
  }

  namespace {
    template <typename T>
    bool is(const DateTimeInterval& d) {
      using R = typename T::RatioToSeconds;
      return d.numerator() == R::num && d.denominator() == R::den;
    }
  }

  std::ostream& operator<<(std::ostream& os, const DateTimeInterval& d) {
    os << d.value() << ' ';
    if (is<Years>(d)) {
      os << GetTimeUnitName<Years>::Value;
    } else if (is<Months>(d)) {
      os << GetTimeUnitName<Months>::Value;
    } else if (is<Weeks>(d)) {
      os << GetTimeUnitName<Weeks>::Value;
    } else if (is<Days>(d)) {
      os << GetTimeUnitName<Days>::Value;
    } else if (is<Hours>(d)) {
      os << GetTimeUnitName<Hours>::Value;
    } else if (is<Minutes>(d)) {
      os << GetTimeUnitName<Minutes>::Value;
    } else if (is<Seconds>(d)) {
      os << GetTimeUnitName<Seconds>::Value;
    } else if (is<Milliseconds>(d)) {
      os << GetTimeUnitName<Milliseconds>::Value;
    } else if (is<Microseconds>(d)) {
      os << GetTimeUnitName<Microseconds>::Value;
    } else if (is<Nanoseconds>(d)) {
      os << GetTimeUnitName<Nanoseconds>::Value;
    } else {
      os << " * " << d.numerator() << "/" << d.denominator() << " seconds";
    }
    return os;
  }

  struct timeval
  DateTimeInterval::to_timeval() const {
    struct timeval tv;
    auto us = microseconds().repr_.count();
    auto s = us / 10000000;
    us %= 10000000;
    tv.tv_sec = s;
    tv.tv_usec = us;
    return tv;
  }
}

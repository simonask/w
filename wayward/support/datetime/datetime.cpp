#include <wayward/support/datetime.hpp>
#include <wayward/support/datetime/timezone.hpp>

#include <ctime>
#include <cassert>

#include <array>

#include <time.h>

namespace wayward {
  using namespace units;

  const Timezone Timezone::UTC = Timezone();

  constexpr char GetTimeUnitName<Years>::Value[];
  constexpr char GetTimeUnitName<Months>::Value[];
  constexpr char GetTimeUnitName<Weeks>::Value[];
  constexpr char GetTimeUnitName<Days>::Value[];
  constexpr char GetTimeUnitName<Hours>::Value[];
  constexpr char GetTimeUnitName<Minutes>::Value[];
  constexpr char GetTimeUnitName<Seconds>::Value[];
  constexpr char GetTimeUnitName<Milliseconds>::Value[];
  constexpr char GetTimeUnitName<Microseconds>::Value[];
  constexpr char GetTimeUnitName<Nanoseconds>::Value[];

  namespace {
    struct Call_tzset {
      Call_tzset() {
        ::tzset();
      }
    };

    static Call_tzset g_call_tzset;
  }

  DateTime DateTime::operator+(const DateTimeInterval& interval) const {
    DateTime copy = *this;
    copy += interval;
    return copy;
  }

  DateTime DateTime::operator-(const DateTimeInterval& interval) const {
    DateTime copy = *this;
    copy -= interval;
    return copy;
  }

  DateTimeInterval DateTime::operator-(DateTime other) const {
    return Nanoseconds{repr_ - other.repr_};
  }

  DateTime& DateTime::operator+=(const DateTimeInterval& interval) {
    DateTimeArithmetic<DateTimeInterval>::adjust(*this, interval);
    return *this;
  }

  DateTime& DateTime::operator-=(const DateTimeInterval& interval) {
    DateTimeArithmetic<DateTimeInterval>::adjust(*this, -interval);
    return *this;
  }

  namespace {
    bool
    is_leap_year(int64_t year) {
      // It's leap year every 4 years, except every 100 years, but then again every 400 years.
      // Source: http://en.wikipedia.org/wiki/Leap_year
      return ((year % 4) == 0) && (((year % 100) != 0) || ((year % 400) == 0));
    }

    uint32_t
    days_in_month(int64_t year, int64_t month) {
      int64_t month_sign = month < 0 ? -1 : 1;

      month *= month_sign; // abs
      year += month_sign * ((month - 1) / 12);
      month = ((month - 1) % 12) + 1;
      month *= month_sign; // undo abs

      switch (month) {
        case 1: return 31;
        case 2: return is_leap_year(year) ? 29 : 28;
        case 3: return 31;
        case 4: return 30;
        case 5: return 31;
        case 6: return 30;
        case 7: return 31;
        case 8: return 31;
        case 9: return 30;
        case 10: return 31;
        case 11: return 30;
        case 12: return 31;
        default: assert(false); // Algorithm error.
      }
    }

    struct tm
    calendar_values_to_tm(const DateTime::CalendarValues& cal) {
      struct tm t = {0};
      t.tm_year = cal.year - 1900;

      // TODO: timegm gives an error when wrapping old dates, for some weird reason
      t.tm_mon = cal.month - 1;
      t.tm_mday = cal.day;
      t.tm_hour = cal.hour;
      t.tm_min = cal.minute;
      t.tm_sec = cal.second;
      t.tm_zone = const_cast<char*>(cal.timezone.zone.data());
      return t;
    }

    DateTime::CalendarValues
    tm_to_calendar_values(const struct tm& t) {
      DateTime::CalendarValues cal;
      cal.year = t.tm_year + 1900;
      cal.month = t.tm_mon + 1;
      cal.day = t.tm_mday;
      cal.hour = t.tm_hour;
      cal.minute = t.tm_min;
      cal.second = t.tm_sec;
      cal.timezone.zone = t.tm_zone;
      return cal;
    }

    Nanoseconds local_tm_to_utc_epoch(struct tm t) {
      auto time_us = std::chrono::system_clock::from_time_t(::mktime(&t));
      return Nanoseconds{time_us.time_since_epoch()};
    }

    Nanoseconds local_calendar_values_to_utc_epoch(const DateTime::CalendarValues& cal) {
      // TODO: The time zone must be the local timezone, otherwise it won't work :(
      struct tm t = calendar_values_to_tm(cal);
      return local_tm_to_utc_epoch(t);
      // TODO: Add sub-millisecond precision.
    }

    struct tm utc_epoch_to_tm_utc(Nanoseconds ns) {
      // Standard std::chrono::time_point only understands microsecond precision.
      auto us = ns.repr_.count() / 1000;
      std::chrono::time_point<std::chrono::system_clock> us_repr {std::chrono::microseconds(us)};
      auto from_epoch = std::chrono::system_clock::to_time_t(us_repr);
      struct tm t;
      ::gmtime_r(&from_epoch, &t);
      return t;
    }

    struct tm utc_epoch_to_tm_local(Nanoseconds ns) {
      // Standard std::chrono::time_point only understands microsecond precision.
      auto us = ns.repr_.count() / 1000;
      std::chrono::time_point<std::chrono::system_clock> us_repr {std::chrono::microseconds(us)};
      auto from_epoch = std::chrono::system_clock::to_time_t(us_repr);
      struct tm t;
      ::localtime_r(&from_epoch, &t);
      return t;
    }

    DateTime::CalendarValues
    utc_epoch_to_local_calendar_values(Nanoseconds ns) {
      struct tm t = utc_epoch_to_tm_local(ns);
      return tm_to_calendar_values(t);
    }
  }

  DateTime::CalendarValues DateTime::as_calendar_values() const {
    return utc_epoch_to_local_calendar_values(repr_.time_since_epoch());
  }

  DateTime DateTime::at(Timezone tz, int32_t year, int32_t month, int32_t d, int32_t h, int32_t m, int32_t s, int32_t ms, int32_t us, int32_t ns) {
    CalendarValues cal;
    cal.year = year;
    cal.month = month;
    cal.day = d;
    cal.hour = h;
    cal.minute = m;
    cal.second = s;
    cal.millisecond = ms;
    cal.microsecond = us;
    cal.nanosecond = ns;
    cal.timezone = tz;
    return at(cal);
  }

  DateTime DateTime::at(int32_t year, int32_t month, int32_t d, int32_t h, int32_t m, int32_t s, int32_t ms, int32_t us, int32_t ns) {
    return at(clock().timezone(), year, month, d, h, m, s, ms, us, ns);
  }

  DateTime DateTime::at(const CalendarValues& cal) {
    return DateTime{DateTime::Repr{local_calendar_values_to_utc_epoch(cal)}};
  }

  Seconds DateTime::unix_timestamp() const {
    return Seconds{repr_.time_since_epoch().count() / 1000000};
  }

  int32_t DateTime::year() const {
    auto cal = as_calendar_values();
    return cal.year;
  }

  int32_t DateTime::month() const {
    auto cal = as_calendar_values();
    return cal.month;
  }

  int32_t DateTime::day() const {
    auto cal = as_calendar_values();
    return cal.day;
  }

  int32_t DateTime::hour() const {
    auto cal = as_calendar_values();
    return cal.hour;
  }

  int32_t DateTime::minute() const {
    auto cal = as_calendar_values();
    return cal.minute;
  }

  int32_t DateTime::second() const {
    auto cal = as_calendar_values();
    return cal.second;
  }

  int32_t DateTime::millisecond() const {
    auto cal = as_calendar_values();
    return cal.millisecond;
  }

  int32_t DateTime::microsecond() const {
    auto cal = as_calendar_values();
    return cal.microsecond;
  }

  int32_t DateTime::nanosecond() const {
    auto cal = as_calendar_values();
    return cal.nanosecond;
  }

  std::string DateTime::strftime(const std::string& fmt) const {
    struct tm t = utc_epoch_to_tm_local(repr_.time_since_epoch());
    std::array<char, 128> buffer;
    size_t len = ::strftime(buffer.data(), buffer.size(), fmt.c_str(), &t);
    return std::string(buffer.data(), len);
  }

  Maybe<DateTime> DateTime::strptime(const std::string& input, const std::string& fmt) {
    struct tm t = {0};
    char* r = ::strptime(input.c_str(), fmt.c_str(), &t);
    if (r == nullptr || r == input.c_str()) {
      // Conversion failed.
      return Nothing;
    } else {
      return DateTime{Repr{local_tm_to_utc_epoch(t)}};
    }
  }

  std::string DateTime::iso8601() const {
    return strftime("%Y-%m-%d %T %z");
  }

  bool DateTimeArithmetic<DateTimeInterval>::is_months(const DateTimeInterval& interval) {
    return interval.numerator() == Months::RatioToSeconds::num && interval.denominator() == Months::RatioToSeconds::den;
  }

  bool DateTimeArithmetic<DateTimeInterval>::is_years(const DateTimeInterval& interval) {
    return interval.numerator() == Years::RatioToSeconds::num && interval.denominator() == Years::RatioToSeconds::den;
  }

  void DateTimeArithmetic<DateTimeInterval>::adjust(DateTime& t, const DateTimeInterval& interval) {
    double v = interval.value();
    bool is_whole_number = (v == (int64_t)v);
    if (is_years(interval) && is_whole_number) {
      t += Years{(int64_t)v};
    } else if (is_months(interval) && is_whole_number) {
      t += Months{(int64_t)v};
    } else {
      /*
        The maximum integer precision of double is 53 bits, but representing a year in
        nanoseconds requires 55 bits.
        To work around this, we start by converting to whole milliseconds, and then doing the remainder.
      */
      auto value_in_seconds = (v * interval.numerator()) / (double)interval.denominator();
      auto value_in_ms = value_in_seconds * 1000;
      int64_t whole_ms = (int64_t)value_in_ms;
      double rem_ms = value_in_ms - whole_ms;
      auto rem_ns = rem_ms * 1000 * 1000;
      int64_t whole_ns = (int64_t)rem_ns;
      auto ns = Nanoseconds{whole_ms * 1000 * 1000 + whole_ns};
      t += ns;
    }
  }

  void DateTimeArithmetic<Years>::adjust(DateTime& t, Years by) {
    DateTime::CalendarValues cal = t.as_calendar_values();
    cal.year += by.repr_.count();

    // If we're adding years from a leap year, we way end up with
    // an off-by-one-day error.
    uint32_t dim = days_in_month(cal.year, cal.month);
    if (cal.day > dim) {
      cal.day = dim;
    }

    t = DateTime::at(cal);
  }

  void DateTimeArithmetic<Months>::adjust(DateTime& t, Months by) {
    DateTime::CalendarValues cal = t.as_calendar_values();
    cal.month += by.repr_.count();

    // Don't leak into next month.
    uint32_t dim = days_in_month(cal.year, cal.month);
    if (cal.day > dim) {
      cal.day = dim;
    }

    t = DateTime::at(cal);
  }
}

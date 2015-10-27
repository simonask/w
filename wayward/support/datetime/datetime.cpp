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
    is_leap_year(Years year) {
      // It's leap year every 4 years, except every 100 years, but then again every 400 years.
      // Source: http://en.wikipedia.org/wiki/Leap_year
      int y = year.count();
      return ((y % 4) == 0) && (((y % 100) != 0) || ((y % 400) == 0));
    }

    Days
    days_in_month(Years year, Months month) {
      int64_t month_sign = month < 0 ? -1 : 1;

      month *= month_sign; // abs
      year += month_sign * ((month - 1_month) / 12);
      month = ((month - 1_month) % 12) + 1_month;
      month *= month_sign; // undo abs

      switch (month.count()) {
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
      t.tm_year = (cal.year - 1900_years).count();

      // TODO: timegm gives an error when wrapping old dates, for some weird reason
      t.tm_mon = (cal.month - 1_month).count();
      t.tm_mday = cal.day.count();
      t.tm_hour = cal.hour.count();
      t.tm_min = cal.minute.count();
      t.tm_sec = cal.second.count();
      t.tm_zone = const_cast<char*>(cal.timezone.zone.data());
      return t;
    }

    DateTime::CalendarValues
    tm_to_calendar_values(const struct tm& t) {
      DateTime::CalendarValues cal;
      cal.year = Years{t.tm_year + 1900};
      cal.month = Months{t.tm_mon + 1};
      cal.day = Days{t.tm_mday};
      cal.hour = Hours{t.tm_hour};
      cal.minute = Minutes{t.tm_min};
      cal.second = Seconds{t.tm_sec};
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
      Nanoseconds ns = local_tm_to_utc_epoch(t);
      ns += cal.nanosecond;
      ns += cal.microsecond;
      ns += cal.millisecond;
      return ns;
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
      auto cal = tm_to_calendar_values(t);

      cal.nanosecond = ns % 1000000000;
      cal.microsecond = cal.nanosecond.count() / 1000;
      cal.nanosecond %= 1000;
      cal.millisecond = cal.microsecond.count() / 1000;
      cal.microsecond %= 1000;
      return cal;
    }
  }

  DateTime::CalendarValues DateTime::as_calendar_values() const {
    return utc_epoch_to_local_calendar_values(repr_.time_since_epoch());
  }

  DateTime DateTime::at(Timezone tz, Years year, Months month, Days d, Hours h, Minutes m, Seconds s, Milliseconds ms, Microseconds us, Nanoseconds ns) {
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

  DateTime DateTime::at(Years year, Months month, Days d, Hours h, Minutes m, Seconds s, Milliseconds ms, Microseconds us, Nanoseconds ns) {
    return at(clock().timezone(), year, month, d, h, m, s, ms, us, ns);
  }

  DateTime DateTime::at(const CalendarValues& cal) {
    return DateTime{DateTime::Repr{local_calendar_values_to_utc_epoch(cal)}};
  }

  Seconds DateTime::unix_timestamp() const {
    return Seconds{repr_.time_since_epoch().count() / 1000000};
  }

  Years DateTime::year() const {
    auto cal = as_calendar_values();
    return cal.year;
  }

  Months DateTime::month() const {
    auto cal = as_calendar_values();
    return cal.month;
  }

  Days DateTime::day() const {
    auto cal = as_calendar_values();
    return cal.day;
  }

  Hours DateTime::hour() const {
    auto cal = as_calendar_values();
    return cal.hour;
  }

  Minutes DateTime::minute() const {
    auto cal = as_calendar_values();
    return cal.minute;
  }

  Seconds DateTime::second() const {
    auto cal = as_calendar_values();
    return cal.second;
  }

  Milliseconds DateTime::millisecond() const {
    auto cal = as_calendar_values();
    return cal.millisecond;
  }

  Microseconds DateTime::microsecond() const {
    auto cal = as_calendar_values();
    return cal.microsecond;
  }

  Nanoseconds DateTime::nanosecond() const {
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
    Days dim = days_in_month(cal.year, cal.month);
    if (cal.day > dim) {
      cal.day = dim;
    }

    t = DateTime::at(cal);
  }

  void DateTimeArithmetic<Months>::adjust(DateTime& t, Months by) {
    DateTime::CalendarValues cal = t.as_calendar_values();
    cal.month += by.repr_.count();

    // Don't leak into next month.
    Days dim = days_in_month(cal.year, cal.month);
    if (cal.day > dim) {
      cal.day = dim;
    }

    t = DateTime::at(cal);
  }
}

#include <wayward/support/datetime.hpp>
#include <wayward/support/datetime_private.hpp>

#include <time.h>

#include <array>

namespace wayward {
  namespace {
    static __thread IClock* g_current_clock = nullptr;
  }

  IClock& clock() {
    if (g_current_clock == nullptr) {
      g_current_clock = &SystemClock::get();
    }
    return *g_current_clock;
  }

  void set_clock(IClock* cl) {
    g_current_clock = cl;
  }

  SystemClock& SystemClock::get() {
    static SystemClock instance;
    return instance;
  }

  DateTime SystemClock::now() const {
    return DateTime{std::chrono::system_clock::now()};
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

  DateTime& DateTime::operator+=(const DateTimeInterval& interval) {
    DateTimeArithmetic<DateTimeInterval>::adjust(*this, interval);
    return *this;
  }

  DateTime& DateTime::operator-=(const DateTimeInterval& interval) {
    DateTimeArithmetic<DateTimeInterval>::adjust(*this, -interval);
    return *this;
  }

  namespace {
    struct tm
    calendar_values_to_tm(const DateTime::CalendarValues& cal) {
      struct tm t = {0};
      t.tm_year = cal.year - 1900;
      t.tm_mon = cal.month - 1;
      t.tm_mday = cal.day;
      t.tm_hour = cal.hour;
      t.tm_min = cal.minute;
      t.tm_sec = cal.second;
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
      return cal;
    }

    Nanoseconds calendar_values_to_nanoseconds_from_epoch(const DateTime::CalendarValues& cal) {
      struct tm t = calendar_values_to_tm(cal);

      int64_t microseconds_from_nanoseconds = cal.nanosecond / 1000;
      int64_t nanoseconds = cal.nanosecond % 1000;
      int64_t microseconds = cal.microsecond + microseconds_from_nanoseconds;
      int64_t milliseconds_from_microseconds = microseconds / 1000;
      microseconds %= 1000;
      int64_t milliseconds = cal.millisecond + milliseconds_from_microseconds;
      int64_t seconds_from_milliseconds = milliseconds / 1000;
      t.tm_sec += seconds_from_milliseconds;
      milliseconds %= 1000;

      auto time_us = std::chrono::system_clock::from_time_t(::timegm(&t));
      auto from_epoch_ns = std::chrono::nanoseconds{time_us.time_since_epoch().count() * 1000};
      from_epoch_ns += std::chrono::nanoseconds{nanoseconds};
      from_epoch_ns += std::chrono::microseconds{microseconds};
      from_epoch_ns += std::chrono::milliseconds{milliseconds};
      return from_epoch_ns;
    }

    struct tm nanoseconds_to_tm(Nanoseconds ns) {
      // Standard std::chrono::time_point only understands microsecond precision.
      auto us = ns.repr_.count() / 1000;
      std::chrono::time_point<std::chrono::system_clock> us_repr {std::chrono::microseconds(us)};
      auto from_epoch = std::chrono::system_clock::to_time_t(us_repr);
      struct tm t;
      ::gmtime_r(&from_epoch, &t);
      return t;
    }

    DateTime::CalendarValues
    nanoseconds_from_epoch_to_calendar_values(Nanoseconds ns) {
      DateTime::CalendarValues cal = tm_to_calendar_values(nanoseconds_to_tm(ns));
      auto ns_rem = ns.repr_.count() % 1000;
      auto us = ns.repr_.count() / 1000;
      cal.millisecond = (us / 1000) % 1000;
      cal.microsecond = us % 1000;
      cal.nanosecond = ns_rem;
      return cal;
    }
  }

  DateTime::CalendarValues DateTime::as_calendar_values() const {
    auto ns = repr_.time_since_epoch();
    return nanoseconds_from_epoch_to_calendar_values(ns);
  }

  DateTime DateTime::at(int64_t year, int32_t month, int64_t d, int64_t h, int64_t m, int64_t s, int64_t ms, int64_t us, int64_t ns) {
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
    return at(cal);
  }

  DateTime DateTime::at(const CalendarValues& cal) {
    return DateTime::Repr{calendar_values_to_nanoseconds_from_epoch(cal)};
  }

  Seconds DateTime::unix_timestamp() const {
    auto ns = repr_.time_since_epoch().count();
    return Seconds{ns / 1000000000};
  }

  int64_t DateTime::year() const {
    auto cal = as_calendar_values();
    return cal.year;
  }

  int64_t DateTime::month() const {
    auto cal = as_calendar_values();
    return cal.month;
  }

  int64_t DateTime::day() const {
    auto cal = as_calendar_values();
    return cal.day;
  }

  int64_t DateTime::hour() const {
    auto cal = as_calendar_values();
    return cal.hour;
  }

  int64_t DateTime::minute() const {
    auto cal = as_calendar_values();
    return cal.minute;
  }

  int64_t DateTime::second() const {
    auto cal = as_calendar_values();
    return cal.second;
  }

  int64_t DateTime::millisecond() const {
    auto cal = as_calendar_values();
    return cal.millisecond;
  }

  int64_t DateTime::microsecond() const {
    auto cal = as_calendar_values();
    return cal.microsecond;
  }

  int64_t DateTime::nanosecond() const {
    auto cal = as_calendar_values();
    return cal.nanosecond;
  }

  std::string DateTime::strftime(const std::string& fmt) const {
    struct tm t = nanoseconds_to_tm(repr_.time_since_epoch());
    std::array<char, 1000> buffer;
    size_t len = ::strftime(buffer.data(), buffer.size(), fmt.c_str(), &t);
    return std::string(buffer.data(), len);
  }

  std::string DateTime::iso8601() const {
    return strftime("%Y-%m-%d %H:%M:%S %z");
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
}

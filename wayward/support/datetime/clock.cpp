#include <wayward/support/datetime/clock.hpp>
#include <wayward/support/datetime/datetime.hpp>
#include <wayward/support/datetime/private.hpp>

#include <sys/time.h>

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

  SystemClock::SystemClock() {}

  SystemClock& SystemClock::get() {
    static SystemClock instance;
    return instance;
  }

  DateTime SystemClock::now() const {
    // Note: Not using std::chrono::system_clock::now(), because it doesn't understand timezones.
    struct timeval tv;
    struct timezone tz;
    ::gettimeofday(&tv, &tz);

    Timezone timezone;
    timezone.utc_offset = Seconds{tz.tz_minuteswest * 60};
    timezone.is_dst = tz.tz_dsttime != 0;

    auto ns = std::chrono::seconds{tv.tv_sec} + std::chrono::microseconds{tv.tv_usec};

    return DateTime{DateTime::Repr{ns}};
  }
}

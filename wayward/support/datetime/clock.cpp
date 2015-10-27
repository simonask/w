#include <wayward/support/datetime/clock.hpp>
#include <wayward/support/datetime/datetime.hpp>
#include <wayward/support/datetime/private.hpp>

#include <time.h>

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
    auto n = std::chrono::system_clock::now();
    return DateTime{DateTime::Repr{n}};
  }

  Timezone SystemClock::timezone() const {
    return Timezone{::tzname[0]};
  }
}

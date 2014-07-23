#include "wayward/support/benchmark.hpp"

#include <sys/resource.h>

namespace wayward {
  DateTime PerformanceClock::now() const {
    struct rusage u;
    ::getrusage(RUSAGE_SELF, &u);
    Seconds s { u.ru_utime.tv_sec + u.ru_stime.tv_sec };
    Microseconds us { u.ru_utime.tv_usec + u.ru_stime.tv_usec };
    Nanoseconds ns = us + s;
    return DateTime{DateTime::Repr{ns}};
  }

  Timezone PerformanceClock::timezone() const {
    // Dummy.
    return Timezone{};
  }

  PerformanceClock& PerformanceClock::get() {
    static PerformanceClock clock;
    return clock;
  }

  Benchmark::Benchmark() {
    start();
  }

  Benchmark::~Benchmark() {
    finish();
  }

  void Benchmark::start() {
    total_.activations = 1;
    total_.begin = PerformanceClock::get().now();
    total_.accum = Microseconds{0};
  }

  DateTimeInterval Benchmark::finish() {
    if (total_.activations) {
      auto end = PerformanceClock::get().now();
      total_.accum += end - total_.begin;
      --total_.activations;
    }
    return total();
  }

  DateTimeInterval Benchmark::total() const {
    return total_.accum;
  }

  DateTimeInterval Benchmark::measure(std::function<void()> f) {
    Benchmark bm;
    f();
    return bm.finish();
  }

  std::map<std::string, DateTimeInterval> Benchmark::scopes() const {
    std::map<std::string, DateTimeInterval> results;
    for (auto& pair: scopes_) {
      results[pair.first] = pair.second.accum;
    }
    return std::move(results);
  }

  void Benchmark::enter_scope(std::string name) {
    auto it = scopes_.find(name);
    if (it == scopes_.end()) {
      it = scopes_.insert(std::make_pair(name, Measurement{})).first;
    }
    if (it->second.activations == 0) {
      it->second.begin = PerformanceClock::get().now();
    }
    ++it->second.activations;
  }

  void Benchmark::exit_scope(std::string name) {
    auto it = scopes_.find(name);
    if (it != scopes_.end()) {
      --it->second.activations;
      if (it->second.activations == 0) {
        auto now = PerformanceClock::get().now();
        it->second.accum += now - it->second.begin;
      }
    }
  }
}

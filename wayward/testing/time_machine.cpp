#include <wayward/testing/time_machine.hpp>
#include <wayward/support/datetime/private.hpp>
#include <wayward/support/maybe.hpp>

namespace wayward {
  namespace testing {
    struct TimeMachine::Impl {
      IClock* previous_clock;
      Maybe<DateTimeInterval> offset;
      Maybe<DateTime> frozen_at;
    };

    TimeMachine::TimeMachine() : impl(new Impl) {
      impl->previous_clock = &wayward::clock();
      wayward::set_clock(this);
    }

    TimeMachine::~TimeMachine() {
      wayward::set_clock(impl->previous_clock);
    }

    DateTime TimeMachine::now() const {
      DateTime t;
      if (impl->frozen_at) {
        t = *impl->frozen_at;
      } else {
        t = impl->previous_clock->now();
      }

      if (impl->offset) {
        t += *impl->offset;
      }
      return t;
    }

    void TimeMachine::freeze(DateTime at) {
      impl->frozen_at = at;
    }

    void TimeMachine::freeze(DateTime at, std::function<void()> callback) {
      auto before = impl->frozen_at;
      impl->frozen_at = at;
      callback();
      impl->frozen_at = before;
    }

    void TimeMachine::travel(DateTimeInterval offset) {
      impl->offset = offset;
    }

    void TimeMachine::travel(DateTimeInterval offset, std::function<void()> callback) {
      auto before = impl->offset;
      impl->offset = offset;
      callback();
      impl->offset = before;
    }

    void TimeMachine::return_to_the_present() {
      impl->offset = Nothing;
      impl->frozen_at = Nothing;
    }
  }
}

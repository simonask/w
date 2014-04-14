#pragma once
#ifndef WAYWARD_TESTING_TIME_MACHINE_HPP_INCLUDED
#define WAYWARD_TESTING_TIME_MACHINE_HPP_INCLUDED

#include <wayward/support/datetime.hpp>
#include <memory>
#include <functional>

namespace wayward {
  namespace testing {
    struct TimeMachine : IClock {
      TimeMachine();
      virtual ~TimeMachine();

      // IClock interface
      DateTime now() const override;

      // TimeMachine interface
      void freeze(DateTime);
      void freeze(DateTime, std::function<void()> callback);
      void travel(DateTimeInterval offset);
      void travel(DateTimeInterval offset, std::function<void()> callback);

      void return_to_the_present();

      struct Impl;
      std::unique_ptr<Impl> impl;
    };
  }
}

#endif // WAYWARD_TESTING_TIME_MACHINE_HPP_INCLUDED

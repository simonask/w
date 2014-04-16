#pragma once
#ifndef WAYWARD_SUPPORT_DATETIME_CLOCK_HPP_INCLUDED
#define WAYWARD_SUPPORT_DATETIME_CLOCK_HPP_INCLUDED

namespace wayward {
  struct DateTime;

  struct IClock {
    virtual ~IClock() {}
    virtual DateTime now() const = 0;
  };

  IClock& clock();

  struct SystemClock : IClock {
    DateTime now() const final;
    static SystemClock& get();
  private:
    SystemClock();
  };
}

#endif // WAYWARD_SUPPORT_DATETIME_CLOCK_HPP_INCLUDED

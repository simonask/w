#pragma once
#ifndef WAYWARD_SUPPORT_DATETIME_TIMEZONE_HPP_INCLUDED
#define WAYWARD_SUPPORT_DATETIME_TIMEZONE_HPP_INCLUDED

#include <wayward/support/datetime/duration_units.hpp>

namespace wayward {
  struct Timezone {
    static const Timezone UTC;

    Seconds utc_offset = Seconds{0};
    bool is_dst = false;
  };
}

#endif // WAYWARD_SUPPORT_DATETIME_TIMEZONE_HPP_INCLUDED

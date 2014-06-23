#pragma once
#ifndef WAYWARD_SUPPORT_DATETIME_TIMEZONE_HPP_INCLUDED
#define WAYWARD_SUPPORT_DATETIME_TIMEZONE_HPP_INCLUDED

#include <string>

#include <wayward/support/datetime/duration_units.hpp>

namespace wayward {
  struct Timezone {
    static const Timezone UTC;
    Timezone() {}
    Timezone(const Timezone&) = default;
    explicit Timezone(std::string name) : zone(std::move(name)) {}

    std::string zone;
  };
}

#endif // WAYWARD_SUPPORT_DATETIME_TIMEZONE_HPP_INCLUDED

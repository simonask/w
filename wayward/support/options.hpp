#pragma once
#ifndef WAYWARD_SUPPORT_OPTIONS_HPP_INCLUDED
#define WAYWARD_SUPPORT_OPTIONS_HPP_INCLUDED

#include <wayward/support/data_franca/spectator.hpp>
#include <wayward/support/data_franca/object.hpp>

namespace wayward {
  using Options = std::map<std::string, data_franca::Spectator>;
}

#endif // WAYWARD_SUPPORT_OPTIONS_HPP_INCLUDED

#pragma once
#ifndef WAYWARD_SUPPORT_PLUGIN_HPP_INCLUDED
#define WAYWARD_SUPPORT_PLUGIN_HPP_INCLUDED

#include <wayward/support/error.hpp>

namespace wayward {
  struct PluginError : Error {
    PluginError(const std::string& msg) : Error(msg) {}
  };

  void load_plugin(std::string name);
}

#endif // WAYWARD_SUPPORT_PLUGIN_HPP_INCLUDED

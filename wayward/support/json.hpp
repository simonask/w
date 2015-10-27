#pragma once
#ifndef WAYWARD_SUPPORT_JSON_HPP_INCLUDED
#define WAYWARD_SUPPORT_JSON_HPP_INCLUDED

#include <wayward/support/data_franca/spectator.hpp>
#include <wayward/support/data_franca/adapters.hpp>
#include <wayward/support/error.hpp>

namespace wayward {
  struct JSONSerializationError : Error {
    JSONSerializationError(const char* what) : Error(what) {}
  };

  enum class JSONMode {
    Compact,
    HumanReadable,
  };

  std::string escape_json(const std::string& input);
  std::string as_json(const data_franca::Spectator& data, JSONMode mode = JSONMode::Compact);
}

#endif // WAYWARD_SUPPORT_JSON_HPP_INCLUDED

#pragma once
#ifndef WAYWARD_SUPPORT_JSON_HPP_INCLUDED
#define WAYWARD_SUPPORT_JSON_HPP_INCLUDED

#include <wayward/support/node.hpp>
#include <stdexcept>

namespace wayward {
  struct JSONSerializationError : std::runtime_error {
    JSONSerializationError(const char* what) : std::runtime_error(what) {}
  };

  enum class JSONMode {
    Compact,
    HumanReadable,
  };

  std::string escape_json(const std::string& input);
  std::string as_json(const Node& data, JSONMode mode = JSONMode::Compact);
}

#endif // WAYWARD_SUPPORT_JSON_HPP_INCLUDED

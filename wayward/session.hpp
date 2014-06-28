#pragma once
#ifndef WAYWARD_SESSION_HPP_INCLUDED
#define WAYWARD_SESSION_HPP_INCLUDED

#include <wayward/support/any.hpp>

namespace wayward {
  struct Session {
    std::map<std::string, Any> data_;
    std::map<std::string, Any> flash_;

    Maybe<std::string> flash(const std::string& key) const;
    void flash(std::string key, Any value);
  };
}

#endif // WAYWARD_SESSION_HPP_INCLUDED

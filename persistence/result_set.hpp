#pragma once
#ifndef PERSISTENCE_RESULT_SET_HPP_INCLUDED
#define PERSISTENCE_RESULT_SET_HPP_INCLUDED

#include <string>
#include <vector>

#include <wayward/support/maybe.hpp>

namespace persistence {
  using wayward::Maybe;

  struct IResultSet {
    virtual ~IResultSet() {}

    virtual size_t width() const = 0; // Number of columns
    virtual size_t height() const = 0; // Number of rows
    virtual std::vector<std::string> columns() const = 0;
    virtual bool is_null_at(size_t idx, const std::string& col) const = 0;
    virtual Maybe<std::string> get(size_t idx, const std::string& col) const = 0;
  };
}

#endif // PERSISTENCE_RESULT_SET_HPP_INCLUDED

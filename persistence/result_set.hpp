#pragma once
#ifndef PERSISTENCE_RESULT_SET_HPP_INCLUDED
#define PERSISTENCE_RESULT_SET_HPP_INCLUDED

#include <string>
#include <deque>

namespace persistence {
  struct IResultSet {
    virtual ~IResultSet() {}

    virtual size_t width() const = 0; // Number of columns
    virtual size_t height() const = 0; // Number of rows
    virtual std::string column_at(size_t idx) const = 0;
  };
}

#endif // PERSISTENCE_RESULT_SET_HPP_INCLUDED

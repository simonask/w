#pragma once
#ifndef PERSISTENCE_TEST_RESULT_SET_MOCK_HPP_INCLUDED
#define PERSISTENCE_TEST_RESULT_SET_MOCK_HPP_INCLUDED

#include <persistence/result_set.hpp>
#include <wayward/support/maybe.hpp>

#include <algorithm>

namespace persistence {
  namespace test {
    using wayward::Maybe;
    using wayward::Nothing;

    struct ResultSetMock : IResultSet {
      size_t width() const { return columns_.size(); }
      size_t height() const { return rows_.size(); }
      std::vector<std::string> columns() const { return columns_; }
      bool is_null_at(size_t idx, const std::string& col) const;
      Maybe<std::string> get(size_t idx, const std::string& col) const;

      std::vector<std::string> columns_;
      std::vector<std::vector<Maybe<std::string>>> rows_;

      Maybe<std::string> value_at(size_t idx, const std::string& col) const;
    };

    inline Maybe<std::string> ResultSetMock::value_at(size_t idx, const std::string& col) const {
      auto it = std::find(columns_.begin(), columns_.end(), col);
      if (it == columns_.end()) return Nothing;
      if (idx >= height()) return Nothing;
      size_t c = it - columns_.begin();
      auto& row = rows_[idx];
      if (c >= row.size()) return Nothing;
      return row[c];
    }

    inline bool ResultSetMock::is_null_at(size_t idx, const std::string& col) const {
      auto m = value_at(idx, col);
      return !m;
    }

    inline Maybe<std::string> ResultSetMock::get(size_t idx, const std::string& col) const {
      return value_at(idx, col);
    }
  }
}

#endif // PERSISTENCE_TEST_RESULT_SET_MOCK_HPP_INCLUDED

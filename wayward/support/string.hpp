#pragma once
#ifndef WAYWARD_SUPPORT_STRING_HPP_INCLUDED
#define WAYWARD_SUPPORT_STRING_HPP_INCLUDED

#include <vector>
#include <string>

namespace wayward {
  std::vector<std::string> split(const std::string& input, const std::string& delimiter);
  std::vector<std::string> split(const std::string& input, const std::string& delimiter, size_t max_groups);
}

#endif // WAYWARD_SUPPORT_STRING_HPP_INCLUDED

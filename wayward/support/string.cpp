#include "wayward/support/string.hpp"

namespace wayward {
  std::vector<std::string> split(const std::string& input, const std::string& delim) {
    if (delim.size() == 0) {
      return {input};
    }

    std::vector<std::string> result;
    size_t p = 0;
    size_t i = 0;
    while (i < input.size() - delim.size()) {
      if (input.substr(i, delim.size()) == delim) {
        result.push_back(input.substr(p, i));
        p = i + delim.size();
        i = p;
      } else {
        ++i;
      }
    }

    if (p < input.size()) {
      result.push_back(input.substr(p));
    }

    return std::move(result);
  }

  std::vector<std::string> split(const std::string& input, const std::string& delim, size_t max_groups) {
    if (delim.size() == 0) {
      return {input};
    }

    std::vector<std::string> result;
    size_t p = 0;
    size_t i = 0;
    while (i < input.size() - delim.size() && result.size() < max_groups) {
      if (input.substr(i, delim.size()) == delim) {
        result.push_back(input.substr(p, i));
        p = i + delim.size();
        i = p;
      } else {
        ++i;
      }
    }

    if (p < input.size()) {
      result.push_back(input.substr(p));
    }

    return std::move(result);
  }
}

#include <wayward/support/format.hpp>
#include <map>
#include <string>
#include <ostream>

namespace wayward {
  void stream_interpolate_indexed(std::ostream& os, const std::string& fmt, const Formattable* formatters, unsigned int num_formatters) {
    static const std::regex find_placeholders {"\\{(\\d+)\\}"};
    wayward::regex_replace_stream(os, fmt, find_placeholders, [&](std::ostream& os, const MatchResults& match) {
      std::stringstream ss(std::string{match[1].first, match[1].second});
      unsigned int idx = num_formatters;
      ss >> idx;
      if (idx >= num_formatters) {
        std::copy(match[0].first, match[0].second, std::ostreambuf_iterator<char>(os));
      } else {
        formatters[idx].formatter->write(os, std::string{match[1].first, match[1].second});
      }
    });
  }

  void stream_interpolate_named(std::ostream& os, const std::string& fmt, const FormattableParameters& params) {
    static const std::regex find_placeholders {"\\{([\\w\\d]+)\\}"};
    wayward::regex_replace_stream(os, fmt, find_placeholders, [&](std::ostream& os, const MatchResults& match) {
      std::string key {match[1].first, match[1].second};
      auto it = params.find(key);
      if (it != params.end()) {
        it->second.formatter->write(os, key);
      } else {
        std::copy(match[0].first, match[0].second, std::ostreambuf_iterator<char>(os));
      }
    });
  }
}

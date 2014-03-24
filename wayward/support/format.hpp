#ifndef W_FORMAT_HPP_INCLUDED
#define W_FORMAT_HPP_INCLUDED

#include <sstream>
#include <utility>
#include <algorithm>
#include <regex>
#include <array>
#include <map>

namespace wayward {
  struct IFormatter {
    virtual ~IFormatter() {}
    virtual void write(std::ostream& os, const std::string& fmt) const = 0;
    virtual IFormatter* duplicate() const = 0;
  };

  template <typename T>
  struct Formatter : IFormatter {
    const T& object;
    Formatter(const T& object) : object(object) {}
    virtual void write(std::ostream& os, const std::string& fmt) const {
      os << object;
    }
    virtual IFormatter* duplicate() const {
      return new Formatter<T>(object);
    }
  };

  struct Formattable {
    Formattable() {}
    Formattable(Formattable&& other) = default;
    template <typename T>
    Formattable(const T& object) {
      formatter = std::unique_ptr<IFormatter>(new Formatter<T>{object});
    }
    // This needs to be copy-constructible because std::initializer_list only allows const-access,
    // so we can't move out values. :(
    Formattable(const Formattable& other) : formatter(other.formatter->duplicate()) {}
    Formattable& operator=(Formattable&& other) = default;
    Formattable& operator=(const Formattable& other) {
      formatter = std::unique_ptr<IFormatter>(other.formatter->duplicate());
      return *this;
    }
    std::unique_ptr<IFormatter> formatter;
  };

  using FormattableParameters = std::map<std::string, Formattable>;

  using MatchResults = std::match_results<std::string::const_iterator>;
  /*
    This is a variant of std::regex_replace that gets its replacement value
    from the return value of a function instead of a plain string.
  */
  template <class OS, class Func>
  void regex_replace_stream(OS& os, const std::string& haystack, const std::regex& rx, Func replacement, std::regex_constants::match_flag_type flags = std::regex_constants::match_default) {
    auto p = haystack.begin();
    auto end = haystack.end();
    auto inserter = std::ostreambuf_iterator<char>(os);
    MatchResults match;
    while (true) {
      if (std::regex_search(p, end, match, rx, flags)) {
        std::copy(p, match[0].first, inserter);
        replacement(os, match);
        p = match[0].second;
      } else {
        break;
      }
    }
    std::copy(p, end, inserter);
  }

  void stream_interpolate_indexed(std::ostream& os, const std::string& fmt, const Formattable* formatters, unsigned int num_formatters);
  void stream_interpolate_named(std::ostream& os, const std::string& fmt, const FormattableParameters& params);

  /*
    Interpolate string with indexed placeholders:

    wayward::format("{0}, {1}!", "Hello", "World");
    // => "Hello, World!"
  */
  template <typename... Rest>
  std::string format(const std::string& fmt, Formattable first, Rest&&... rest) {
    std::array<Formattable, sizeof...(Rest) + 1> formatters {{{std::move(first)}, {rest}...}};
    std::stringstream ss;
    stream_interpolate_indexed(ss, fmt, formatters.data(), sizeof...(Rest) + 1);
    return ss.str();
  }

  /*
    Interpolate string with named placeholders:

    wayward::format("{greeting}, {entity}!", {{"greeting", "Hello"}, {"entity", "World"}});
    // => "Hello, World!"
  */
  inline std::string format(const std::string& fmt, const FormattableParameters& params) {
    std::stringstream ss;
    stream_interpolate_named(ss, fmt, params);
    return ss.str();
  }

  /*
    Just returns 'fmt' unchanged.
  */
  inline std::string format(const std::string& fmt) {
    return fmt;
  }
}

#endif /* end of include guard: SYMBOL */

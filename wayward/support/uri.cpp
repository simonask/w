#include "wayward/support/uri.hpp"
#include <regex>
#include <wayward/support/format.hpp>

#include <cstdlib>
#include <cassert>

namespace wayward {
  Maybe<URI> URI::parse(const std::string& input) {
    /*
      Groups:
      (scheme) :// ( (username) (:(password))? @)? (hostname) (:(port))? (path)? (\?(query))? (#(fragment))?
      1 = scheme
      3 = username
      5 = password
      6 = hostname
      8 = port
      9 = path
      10 = query
      12 = fragment
    */
    static const std::regex uri_matcher { "^([\\w]+)\\://(([^:@]+)(\\:([^@]+))?@)?([^/:?#]+)(\\:(\\d+))?([^?#]+)?(\\?([^#]*))?(#(.*))?$" };
    MatchResults results;
    if (std::regex_match(input, results, uri_matcher)) {
      URI uri;
      uri.scheme = results[1];
      uri.username = results[3];
      uri.password = results[5];
      uri.host = results[6];
      if (results.length(8)) {
        std::stringstream ss { results[8] };
        ss >> uri.port;
      } else {
        uri.port = -1;
      }
      uri.path = results[9];
      uri.query = results[10];
      uri.fragment = results[12];
      return uri;
    } else {
      return Nothing;
    }
  }

  namespace {
    int hexdigit_to_integer(int digit) {
      if (digit >= 'a')
        digit -= 'a' - 'A';
      if (digit >= 'A')
        digit -= 'A' - 10;
      else
        digit -= '0';
      return digit;
    }
  }

  std::string
  URI::decode(const std::string& input) {
    std::string result;
    result.reserve(input.size()); // We know that output size <= input size
    size_t ilen = input.size();
    for (size_t i = 0; i < ilen; ++i) {
      if (i < ilen - 2 && input[i] == '%') {
        int a = input[i+1];
        int b = input[i+2];
        if (std::isxdigit(a) && std::isxdigit(b)) {
          a = hexdigit_to_integer(a);
          b = hexdigit_to_integer(b);
          int c = a * 0x10 + b;
          assert(c < 256); // Consistency error!
          result.push_back((char)c);
          i += 2;
          continue;
        }
      }

      if (input[i] == '+')
        result.push_back(' ');
      else
        result.push_back(input[i]);
    }
    return std::move(result);
  }

  std::string URI::to_string() const {
    std::stringstream ss;
    ss << scheme << "://";
    if (username.size()) {
      ss << username;
      if (password.size()) {
        ss << ':' << password;
      }
      ss << '@';
    }
    ss << host << path << query << fragment;
    return ss.str();
  }
}

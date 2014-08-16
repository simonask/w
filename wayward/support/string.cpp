#include "wayward/support/string.hpp"

#include <assert.h>

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

  std::string trim(const std::string& input) {
    if (input.size() == 0)
      return input;

    std::string::size_type p0 = 0;
    std::string::size_type p1 = input.size() - 1;
    while (::isspace(input[p0])) ++p0;
    while (::isspace(input[p1])) --p1;

    return input.substr(p0, p1 - p0 + 1);
  }

  namespace utf8 {
    size_t char_length(const char* utf8, size_t max_bytes) {
      if (max_bytes) {
        char ch = utf8[0];
        if (utf8::is_7bit_ascii(ch))
          return 1;
        if (utf8::is_utf8_leading_byte(ch)) {
          if ((ch & 0xfe) == 0xfc) return 6;
          if ((ch & 0xfc) == 0xf8) return 5;
          if ((ch & 0xf8) == 0xf0) return 4;
          if ((ch & 0xf0) == 0xe0) return 3;
          if ((ch & 0xe0) == 0xc0) return 2;
        }
      }
      return 0;
    }

    size_t count_utf8_code_points(const char* utf8, size_t max_bytes) {
      size_t count = 0;
      for (size_t i = 0; i < max_bytes; ++i) {
        if (is_7bit_ascii(utf8[i]) || is_utf8_leading_byte(utf8[i]))
          ++count;
      }
      return count;
    }

    size_t char_count_to_byte_count(const char* utf8, size_t max_bytes, size_t chcount) {
      size_t b = 0;
      size_t c = 0;
      while (b < max_bytes && c < chcount) {
        b += char_length(utf8 + b, max_bytes - b);
        ++c;
      }
      return b;
    }

    bool is_7bit_ascii(char ch) {
      return (ch & 0x80) == 0;
    }

    bool is_utf8_leading_byte(char ch) {
      return (ch & 0xc0) == 0xc0;
    }

    bool is_utf8_byte(char ch) {
      return !is_7bit_ascii(ch);
    }

    bool counts_as_character(char ch) {
      return ((ch & 0x80) == 0) || ((ch & 0xc0) == 0xc0);
    }

    bool contains_utf8(const char* utf8, size_t max_bytes) {
      for (auto p = utf8; p != utf8 + max_bytes; ++p) {
        if (is_utf8_byte(*p)) return true;
      }
      return false;
    }

    char32_t utf8_to_char32(const char* utf8, size_t len) {
      if (len == 0) return 0;
      const char* p = utf8;
      char32_t result = 0;

      char ch = utf8[0];

      if (is_7bit_ascii(ch))
        return ch;

      if (is_utf8_leading_byte(ch)) {
        size_t num_continuation_bytes;
        if ((ch & 0xfe) == 0xfc) {
          num_continuation_bytes = 5;
          result = ch & 0x1;
        }
        else if ((ch & 0xfc) == 0xf8) {
          num_continuation_bytes = 4;
          result = ch & 0x3;
        }
        else if ((ch & 0xf8) == 0xf0) {
          num_continuation_bytes = 3;
          result = ch & 0x7;
        }
        else if ((ch & 0xf0) == 0xe0) {
          num_continuation_bytes = 2;
          result = ch & 0xf;
        }
        else if ((ch & 0xe0) == 0xc0) {
          num_continuation_bytes = 1;
          result = ch & 0x1f;
        }
        else {
          // Invalid UTF-8!
          return '\x7f';
        }

        if (num_continuation_bytes + 1 > len) {
          // Invalid UTF-8!
          return '\x7f';
        }

        size_t i = 0;
        do {
          result <<= 6;
          result |= utf8[i + 1] & 0x3f;
          ++i;
        } while (i < num_continuation_bytes);

        return result;
      } else {
        // Invalid UTF-8!
        return '\x7f';
      }
    }

    size_t char32_to_utf8(char32_t ch, char* p, size_t max_len) {
      assert(max_len >= 6); // Not enough room to contain all characters in pre-2003 UTF-8.

      if (ch >= 0x4000000) {
        p[0] = (ch >> 30) | 0xfc;
        p[1] = ((ch >> 24) & 0x3f) | 0x80;
        p[2] = ((ch >> 18) & 0x3f) | 0x80;
        p[3] = ((ch >> 12) & 0x3f) | 0x80;
        p[4] = ((ch >>  6) & 0x3f) | 0x80;
        p[5] = ( ch        & 0x3f) | 0x80;
        return 6;
      } else
      if (ch >= 0x200000) {
        p[0] = (ch >> 24) | 0xf8;
        p[1] = ((ch >> 18) & 0x3f) | 0x80;
        p[2] = ((ch >> 12) & 0x3f) | 0x80;
        p[3] = ((ch >>  6) & 0x3f) | 0x80;
        p[4] = ( ch        & 0x3f) | 0x80;
        return 5;
      } else
      if (ch >= 0x10000) {
        p[0] = (ch >> 18) | 0xf0;
        p[1] = ((ch >> 12) & 0x3f) | 0x80;
        p[2] = ((ch >>  6) & 0x3f) | 0x80;
        p[3] = ( ch        & 0x3f) | 0x80;
        return 4;
      } else
      if (ch >= 0x800) {
        p[0] = (ch >> 12) | 0xe0;
        p[1] = ((ch >> 6) & 0x3f) | 0x80;
        p[2] = ( ch       & 0x3f) | 0x80;
        return 3;
      } else
      if (ch >= 0x80) {
        p[0] = (ch >> 6) | 0xc0;
        p[1] = (ch & 0x3f) | 0x80;
        return 2;
      } else {
        // Plain 7-bit ASCII:
        p[0] = (char)ch;
        return 1;
      }
    }
  }

  Char::Char(const char* utf8, size_t max_bytes)
  : char_(utf8::utf8_to_char32(utf8, max_bytes))
  {}

  Char::Char(const char* utf8) : Char(utf8, ::strlen(utf8)) {}

  bool Char::is_ascii() const {
    return char_ < 128;
  }

  char32_t Char::codepoint() const {
    return char_;
  }

  String Char::string() const {
    char buffer[6] = {0};
    String::ByteCount len = utf8::char32_to_utf8(char_, buffer, 6);
    return String{buffer, len};
  }

  String::String(std::string s, bool is_ascii) : bytes_(std::move(s)), is_ascii_(is_ascii) {}

  String::String(std::string s)
  : bytes_(std::move(s))
  , is_ascii_(!utf8::contains_utf8(c_str(), size()))
  {}


  String::String(CharCount count, Char ch)
  : is_ascii_(ch.is_ascii())
  {
    for (CharCount i = 0; i < count; ++i) {
      utf8::copy_char_as_utf8(ch, std::back_inserter(bytes_));
    }
    bytes_.shrink_to_fit();
  }

  String::String(const String& other, CharCount pos, CharCount characters) {
    auto bpos = other.char_position_to_byte_position(pos);
    if (characters != NPos) {
      auto epos = other.char_position_to_byte_position(pos + characters, bpos, pos);
      bytes_ = other.bytes_.substr(bpos, epos - bpos);
    } else {
      bytes_ = other.bytes_.substr(bpos);
    }
    is_ascii_ = utf8::contains_utf8(c_str(), size());
  }

  String::String(const char* utf8, ByteCount bc) {
    bytes_ = std::string{utf8, (size_t)bc};
    is_ascii_ = !utf8::contains_utf8(c_str(), bc);
  }

  String::String(const char* utf8)
  : bytes_(utf8)
  , is_ascii_(!utf8::contains_utf8(c_str(), size()))
  {}

  String::String(std::initializer_list<Char> init) : is_ascii_(true) {
    for (auto ch: init) {
      utf8::copy_char_as_utf8(ch, std::back_inserter(bytes_));
      is_ascii_ = is_ascii_ && ch.is_ascii();
    }
    bytes_.shrink_to_fit();
  }

  String::String(std::initializer_list<char> init)
  : bytes_(std::move(init))
  , is_ascii_(!utf8::contains_utf8(c_str(), size()))
  {}

  String::ByteCount String::size() const {
    return bytes_.size();
  }

  String::CharCount String::length() const {
    if (is_ascii_)
      return size();
    return utf8::count_utf8_code_points(c_str(), size());
  }

  const char* String::c_str() const {
    return bytes_.c_str();
  }

  const std::string& String::bytes() const {
    return bytes_;
  }

  std::string& String::bytes() {
    return bytes_;
  }

  Char String::operator[](String::CharCount idx) const {
    return at(idx);
  }

  Char String::at(String::CharCount idx) const {
    ByteCount pos = char_position_to_byte_position(idx);
    return Char{ c_str() + pos, (size_t)(size() - pos) };
  }

  String::iterator String::begin() const {
    return iterator{c_str()};
  }

  String::iterator String::end() const {
    return iterator{c_str() + size()};
  }

  String::ByteCount String::char_position_to_byte_position(CharCount chpos) const {
    return char_position_to_byte_position(chpos, 0, 0);
  }

  String::ByteCount String::char_position_to_byte_position(CharCount chpos, ByteCount start, CharCount count_at_start) const {
    return utf8::char_count_to_byte_count(c_str() + start, size() - start, chpos) + count_at_start;
  }

  String operator+(const String& a, const String& b) {
    return String{a.bytes_ + b.bytes_, a.is_ascii_ && b.is_ascii_};
  }
}

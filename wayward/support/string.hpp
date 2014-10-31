#pragma once
#ifndef WAYWARD_SUPPORT_STRING_HPP_INCLUDED
#define WAYWARD_SUPPORT_STRING_HPP_INCLUDED

#include <vector>
#include <string>

#include <wayward/support/error.hpp>

namespace wayward {
  std::vector<std::string> split(const std::string& input, const std::string& delimiter);
  std::vector<std::string> split(const std::string& input, const std::string& delimiter, size_t max_groups);
  std::string trim(const std::string& input);

  struct String;

  struct Char {
    Char() : char_(0) {}
    Char(char32_t c) : char_(c) {}
    explicit Char(const char* utf8);
    Char(const char* utf8, size_t max_bytes);

    String string() const;
    char32_t codepoint() const;
    size_t size() const;
    bool is_ascii() const;

    char32_t char_;
  };

  inline bool operator==(Char a, Char b) { return a.codepoint() == b.codepoint(); }
  inline bool operator!=(Char a, Char b) { return a.codepoint() != b.codepoint(); }
  inline bool operator<(Char a, Char b) { return a.codepoint() < b.codepoint(); }
  inline bool operator>(Char a, Char b) { return a.codepoint() > b.codepoint(); }
  inline bool operator<=(Char a, Char b) { return a.codepoint() <= b.codepoint(); }
  inline bool operator>=(Char a, Char b) { return a.codepoint() >= b.codepoint(); }

  namespace utf8 {
    size_t char_length(const char* utf8, size_t max_bytes);
    size_t count_utf8_code_points(const char* utf8, size_t max_bytes);
    size_t char_count_to_byte_count(const char* utf8, size_t max_bytes, size_t chcount);
    bool is_7bit_ascii(char ch);
    bool is_utf8_leading_byte(char ch);
    bool is_utf8_byte(char ch);
    bool contains_utf8(const char* utf8, size_t max_bytes);
    bool is_valid_utf8(const char* utf8, size_t max_bytes);
    char32_t utf8_to_char32(const char* utf8, size_t len);
    size_t char32_to_utf8(char32_t ch, char* out_utf8, size_t max_len);

    template <typename OutputIterator>
    void copy_char_as_utf8(Char ch, OutputIterator it) {
      char utf8[6];
      size_t len = char32_to_utf8(ch.codepoint(), utf8, 6);
      for (auto p = utf8; p < utf8 + len; ++p, ++it) {
        *it = *p;
      }
    }

    template <typename OutputIterator>
    void copy_char_as_utf8(char ch, OutputIterator it) {
      *it++ = ch;
    }
  }

  struct InvalidUTF8Error : Error {
    InvalidUTF8Error(const std::string& message) : Error(message) {}
  };

  // A wayward::String is an immutable UTF8-aware string type.
  // (API-compatible with std::string, except for mutating methods).
  struct String {
    using ByteCount = ssize_t;
    using CharCount = ssize_t;
    static constexpr const auto NPos = std::string::npos;

    explicit String(std::string s);
    String(CharCount count, Char ch);
    String(const String&, CharCount pos, CharCount characters = NPos);
    String(const char* utf8, ByteCount count);
    String(const char* utf8);
    String(const String&) = default;
    String(String&&) noexcept = default;
    String(std::initializer_list<Char> init);
    String(std::initializer_list<char> init);

    template <class InputIterator>
    String(InputIterator first, InputIterator last);

    // Slimmed-down std::string interface
    String& operator=(const String&) = default;
    String& operator=(String&&) = default;
    Char at(CharCount position) const;
    Char operator[](CharCount position) const;
    Char front() const;
    Char back() const;
    const char* data() const;
    const char* c_str() const;
    ByteCount size() const;
    bool empty() const;
    CharCount find(const String& needle, CharCount start = 0, CharCount count = NPos) const;
    CharCount rfind(const String& needle, CharCount start = NPos) const;
    CharCount find_first_of(const String& needle, CharCount start = 0, CharCount count = NPos) const;
    CharCount find_first_not_of(const String& needle, CharCount start = 0, CharCount count = NPos) const;
    CharCount find_last_of(const String& needle, CharCount start = NPos) const;
    CharCount find_last_not_of(const String& needle, CharCount start = NPos) const;

    // Own interface:
    const std::string& bytes() const;
    std::string& bytes();
    CharCount length() const; // Number of codepoints.
    struct iterator;
    iterator begin() const;
    iterator end() const;
    bool is_ascii() const;

    String normalized_nfc() const;
    String normalized_nfd() const;
    String normalized_nfkc() const;
    String normalized_nfkd() const;

    std::vector<String> split(const String& delim) const;
    std::vector<String> split(const String& delim, size_t max_groups) const;

    std::string bytes_;
    bool is_ascii_;

  private:
    String(std::string s, bool is_ascii);

    // Allow access to private constructor to avoid rescanning strings for non-ASCII
    // characters on concatenation.
    friend String operator+(const String&, const String&);

    ByteCount char_position_to_byte_position(CharCount chpos) const;
    ByteCount char_position_to_byte_position(CharCount chpos, ByteCount start, CharCount count_at_start) const;
  };

  String operator+(const String&, const String&);

  inline bool operator==(const String& a, const String& b) { return a.bytes_ == b.bytes_; }
  inline bool operator!=(const String& a, const String& b) { return a.bytes_ != b.bytes_; }
  // Warning: These are byte-level orderings. The correct way to
  // sort strings for user presentation is locale-dependent.
  inline bool operator<(const String& a, const String& b) { return a.bytes_ < b.bytes_; }
  inline bool operator>(const String& a, const String& b) { return a.bytes_ > b.bytes_; }
  inline bool operator<=(const String& a, const String& b) { return a.bytes_ <= b.bytes_; }
  inline bool operator>=(const String& a, const String& b) { return a.bytes_ >= b.bytes_; }

  template <typename InputIterator>
  String::String(InputIterator first, InputIterator last) : is_ascii_(true) {
    for (auto it = first; it != last; ++it) {
      is_ascii_ = is_ascii_ && utf8::is_7bit_ascii(*it);
      utf8::copy_char_as_utf8(*it, std::back_inserter(bytes_));
    }
    bytes_.shrink_to_fit();
  }

  struct String::iterator {
    iterator(const iterator&) = default;
    iterator& operator=(const iterator&) = default;

    Char operator*() const { return Char{p_, current_char_length_}; }
    iterator& operator++()    { increment(); return *this; }
    iterator  operator++(int) { auto copy = *this; increment(); return copy; }

    bool operator==(const iterator& other) const { return p_ == other.p_; }
    bool operator!=(const iterator& other) const { return p_ != other.p_; }
  private:
    friend struct String;
    explicit iterator(const char* p) : p_(p) { update(); }
    explicit iterator() : p_(nullptr), current_char_length_(0) {}
    const char* p_;
    size_t current_char_length_;

    void update() {
      current_char_length_ = utf8::char_length(p_, 6);
    }

    void increment() {
      p_ += current_char_length_;
      update();
    }
  };
}

namespace std {
  template <>
  struct hash<::wayward::String> {
    size_t operator()(const ::wayward::String& str) const {
      return std::hash<std::string>()(str.bytes());
    }
  };
}

// TODO: Streams should be encoding-aware.
template <typename OS>
OS& operator<<(OS& os, const wayward::String& str) {
  os << str.bytes();
  return os;
}
template <typename OS>
OS& operator<<(OS& os, const wayward::Char& c) {
  os << c.string();
  return os;
}

#endif // WAYWARD_SUPPORT_STRING_HPP_INCLUDED

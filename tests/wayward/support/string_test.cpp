#include <gtest/gtest.h>
#include <wayward/support/string.hpp>
#include <sstream>

namespace wayward {
  void PrintTo(const wayward::String& str, std::ostream* os) {
    *os << str;
  }

  void PrintTo(const wayward::Char& ch, std::ostream* os) {
    *os << "0x" << std::hex << ch.codepoint();
  }
}

namespace {
  using wayward::String;
  using wayward::Char;
  using wayward::trim;

  TEST(String, trim_returns_identity) {
    std::string a = "aaa";
    EXPECT_EQ("aaa", trim(a));
  }

  TEST(String, trim_strips_leading_spaces) {
    std::string a = " aa";
    EXPECT_EQ("aa", trim(a));
  }

  TEST(String, trim_strips_trailing_spaces) {
    std::string a = "aa ";
    EXPECT_EQ("aa", trim(a));
  }

  TEST(String, trim_does_not_remove_whitespace_from_the_middle) {
    std::string a = " a a ";
    EXPECT_EQ("a a", trim(a));
  }


  TEST(String, recognizes_ascii_character) {
    const char input[] = "A";
    bool ascii = wayward::utf8::is_7bit_ascii(input[0]);
    EXPECT_EQ(true, ascii);
  }

  TEST(String, recognizes_utf8_character) {
    const char input[] = "Ø";
    bool ascii = wayward::utf8::is_7bit_ascii(input[0]);
    EXPECT_EQ(false, ascii);
  }

  TEST(String, finds_length_of_utf8_character) {
    const char input[] = "Ø";
    size_t len = wayward::utf8::char_length(input, sizeof(input));
    EXPECT_EQ(2, len);
  }

  TEST(String, counts_utf8_characters) {
    String str { "æøå" };
    EXPECT_EQ(3, str.length());
    EXPECT_EQ(6, str.size());

    String str2 { "abcæøåxyz" };
    EXPECT_EQ(9, str2.length());
    EXPECT_EQ(12, str2.size());
  }

  TEST(String, access_characters_by_index) {
    String str { "æøå" };
    EXPECT_EQ(Char{"ø"}, str[1]);
    EXPECT_NE(Char{"a"}, str[1]);
  }

  TEST(String, iterates_over_chars) {
    String str { "abcæøåxyz" };
    std::vector<Char> chars;
    for (auto ch: str) {
      chars.push_back(ch);
    }
    EXPECT_EQ(str[0], chars[0]);
    EXPECT_EQ(str[1], chars[1]);
    EXPECT_EQ(str[2], chars[2]);
    EXPECT_EQ(str[3], chars[3]);
    EXPECT_EQ(str[4], chars[4]);
    EXPECT_EQ(str[5], chars[5]);
    EXPECT_EQ(str[6], chars[6]);
    EXPECT_EQ(str[7], chars[7]);
    EXPECT_EQ(str[8], chars[8]);
  }

  TEST(String, concatenates) {
    String a { "Hello, " };
    String b { "World!" };
    EXPECT_EQ("Hello, World!", a + b);
  }

  TEST(String, streams_to_output) {
    std::stringstream ss;
    ss << String{"abc"};
    EXPECT_EQ("abc", ss.str());
  }

  TEST(Char, encodes_NUL) {
    EXPECT_EQ(Char{'\0'}.codepoint(), 0);
  }

  TEST(Char, encodes_ASCII) {
    EXPECT_EQ(Char{'\x42'}.codepoint(), 0x42);
  }

  TEST(Char, encodes_2byte) {
    Char ch {"ø"};
    EXPECT_EQ(ch.string(), String{"ø"});
    EXPECT_EQ(ch.codepoint(), 0xf8);
  }

  TEST(Char, decodes_2byte) {
    Char ch { 0xf8 };
    EXPECT_EQ("ø", ch.string());
    EXPECT_EQ(2, ch.string().size());
    EXPECT_EQ(1, ch.string().length());
  }

  TEST(Char, encodes_3byte) {
    Char ch {"わ"};
    EXPECT_EQ(ch.string(), String{"わ"});
    EXPECT_EQ(ch.codepoint(), 0x308f);
  }

  TEST(Char, decodes_3byte) {
    Char ch { 0x20ac };
    EXPECT_EQ("€", ch.string());
    EXPECT_EQ(3, ch.string().size());
    EXPECT_EQ(1, ch.string().length());
  }

  TEST(Char, encodes_4byte) {
    Char ch {"𐤀"};
    EXPECT_EQ(ch.string(), String{"𐤀"});
    EXPECT_EQ(ch.codepoint(), 0x10900);
  }

  TEST(Char, decodes_4byte) {
    Char ch { 0x10900 };
    EXPECT_EQ("𐤀", ch.string());
    EXPECT_EQ(4, ch.string().size());
    EXPECT_EQ(1, ch.string().length());
  }
}

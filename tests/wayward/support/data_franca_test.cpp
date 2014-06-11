#include <gtest/gtest.h>
#include <wayward/support/data_franca.hpp>

namespace {
  using namespace wayward::data_franca;

  TEST(Spectator, inspects_ints) {
    int n = 123;
    Spectator s { n };
    EXPECT_EQ(DataType::Integer, s.type());
    Integer m;
    bool result = s >> m;
    EXPECT_EQ(true, result);
    EXPECT_EQ(123, m);
  }

  TEST(Spectator, inspects_strings) {
    std::string a = "Hello, World!";
    Spectator s { a };
    EXPECT_EQ(DataType::String, s.type());
    String m;
    bool result = s >> m;
    EXPECT_EQ(true, result);
    EXPECT_EQ("Hello, World!", m);
  }

  TEST(Spectator, inspects_vectors_of_ints) {
    std::vector<int> v {1, 2, 3, 4};
    Spectator s { v };
    EXPECT_EQ(DataType::List, s.type());
    Integer i = 1;
    for (auto it: s) {
      EXPECT_EQ(DataType::Integer, it.type());
      Integer n;
      EXPECT_EQ(true, it >> n);
      EXPECT_EQ(i, n);
      ++i;
    }
  }

  TEST(Spectator, inspects_maps_of_ints) {
    std::map<String, int> m { {"a", 123}, {"b", 567} };
    Spectator s { m };
    EXPECT_EQ(DataType::Dictionary, s.type());
    std::string keys[] = {{"a"}, {"b"}};
    size_t i = 0;
    for (auto it = s.begin(); it != s.end(); ++it) {
      EXPECT_EQ(*it.key(), keys[i]);
      auto ss = *it;
      Integer n;
      EXPECT_EQ(true, ss >> n);
      EXPECT_EQ(n, m[*it.key()]);
      ++i;
    }
  }

  TEST(Mutator, inspects_ints) {
    int n = 123;
    Mutator s { n };
    EXPECT_EQ(DataType::Integer, s.type());
    Integer m;
    bool result = s >> m;
    EXPECT_EQ(true, result);
    EXPECT_EQ(123, m);
  }

  TEST(Mutator, inspects_strings) {
    std::string a = "Hello, World!";
    Mutator s { a };
    EXPECT_EQ(DataType::String, s.type());
    String m;
    bool result = s >> m;
    EXPECT_EQ(true, result);
    EXPECT_EQ("Hello, World!", m);
  }

  TEST(Mutator, inspects_vectors_of_ints) {
    std::vector<int> v {1, 2, 3, 4};
    Mutator s { v };
    EXPECT_EQ(DataType::List, s.type());
    Integer i = 1;
    for (auto it: s) {
      EXPECT_EQ(DataType::Integer, it.type());
      Integer n;
      EXPECT_EQ(true, it >> n);
      EXPECT_EQ(i, n);
      ++i;
    }
  }

  TEST(Mutator, inspects_maps_of_ints) {
    std::map<String, int> m { {"a", 123}, {"b", 567} };
    Mutator s { m };
    EXPECT_EQ(DataType::Dictionary, s.type());
    std::string keys[] = {{"a"}, {"b"}};
    size_t i = 0;
    for (auto it = s.begin(); it != s.end(); ++it) {
      EXPECT_EQ(*it.key(), keys[i]);
      auto ss = *it;
      Integer n;
      EXPECT_EQ(true, ss >> n);
      EXPECT_EQ(n, m[*it.key()]);
      ++i;
    }
  }

  TEST(Mutator, mutates_integer) {
    int n = 123;
    Mutator m { n };
    m << 456;
    EXPECT_EQ(456, n);
  }

  TEST(Mutator, mutates_string) {
    std::string s = "Hello";
    Mutator m { s };
    m << "World";
    EXPECT_EQ("World", s);
  }

  TEST(Mutator, mutates_vector_of_ints) {
    std::vector<int> v = { 1, 2, 3 };
    Mutator m { v };
    m.push_back(4);
    EXPECT_EQ(4, v.size());
    EXPECT_EQ(4, v[3]);
  }

  TEST(Mutator, mutates_dictionary_of_ints) {
    std::map<std::string, int> map;
    map["a"] = 1;
    map["b"] = 2;
    Mutator m { map };
    m["a"] << 3;
    EXPECT_EQ(3, map["a"]);
  }


  TEST(Object, builds_int) {
    Object o { 123 };
    EXPECT_EQ(DataType::Integer, o.type());
    Integer m;
    EXPECT_EQ(true, o >> m);
    EXPECT_EQ(123, m);
  }

  TEST(Object, builds_string) {
    Object o { "Hello, World!" };
    EXPECT_EQ(DataType::String, o.type());
    String s;
    EXPECT_EQ(true, o >> s);
    EXPECT_EQ("Hello, World!", s);
  }

  TEST(Object, builds_vector_of_ints) {
    Object o;
    int numbers[3] = {123, 456, 789};
    for (size_t i = 0; i < 3; ++i) {
      o.push_back(numbers[i]);
    }
    EXPECT_EQ(DataType::List, o.type());
    EXPECT_EQ(3, o.length());
    for (size_t i = 0; i < 3; ++i) {
      Integer n;
      EXPECT_EQ(true, o[i] >> n);
      EXPECT_EQ(numbers[i], n);
    }
  }

  TEST(Object, builds_dictionary_of_ints) {
    Object o;
    std::string keys[] = {"a", "b", "c"};
    int numbers[] = {1, 2, 3};
    for (size_t i = 0; i < 3; ++i) {
      o[keys[i]] = numbers[i];
    }
    EXPECT_EQ(DataType::Dictionary, o.type());
    EXPECT_EQ(3, o.length());
    for (size_t i = 0; i < 3; ++i) {
      Integer n;
      EXPECT_EQ(true, o[keys[i]] >> n);
      EXPECT_EQ(numbers[i], n);
    }
  }
}

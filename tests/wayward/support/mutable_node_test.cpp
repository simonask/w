#include <gtest/gtest.h>
#include <wayward/support/mutable_node.hpp>

namespace {
  using wayward::MutableNode;
  using wayward::NodeType;

  TEST(MutableNode, initializes_with_string_constant) {
    MutableNode str = "Hello, World!";
    EXPECT_EQ(NodeType::String, str.type());
  }

  TEST(MutableNode, initializes_with_integer_constant) {
    MutableNode n = 123;
    EXPECT_EQ(NodeType::Integer, n.type());
  }

  TEST(MutableNode, initialized_with_boolean_constant) {
    MutableNode b = true;
    EXPECT_EQ(NodeType::Boolean, b.type());
  }

  TEST(MutableNode, sets_member_of_dictionary_to_integer_constant) {
    MutableNode dict = MutableNode::dictionary();
    dict["hello"] = 123;
    EXPECT_EQ(NodeType::Integer, dict["hello"].type());
  }

  TEST(MutableNode, sets_member_of_dictionary_to_other_node) {
    MutableNode dict = MutableNode::dictionary();
    MutableNode n = 123;
    dict["hello"] = n;
    EXPECT_EQ(NodeType::Integer, dict["hello"].type());
  }

  TEST(MutableNode, appends_to_list) {
    MutableNode list = MutableNode::list();
    list.push_back(123);
    EXPECT_EQ(NodeType::Integer, list[(size_t)0].type());
  }
}

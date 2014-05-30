#pragma once
#ifndef WAYWARD_SUPPORT_NODE_HPP_INCLUDED
#define WAYWARD_SUPPORT_NODE_HPP_INCLUDED

#include <wayward/support/maybe.hpp>
#include <wayward/support/structured_data_adapters.hpp>

namespace wayward {
  /*
    A Node is any element as part of structured data.

    Think of it as any subtree of JSON data.
  */
  struct Node {
    explicit Node(std::shared_ptr<const IStructuredData> node) : node_(std::move(node)) {}
    Node() {}
    Node(const Node&) = default;
    Node(Node&&) = default;
    Node& operator=(const Node&) = default;
    Node& operator=(Node&&) = default;
    Node(std::map<std::string, Node> dict);
    template <typename T>
    Node(T&& implicitly_adaptable_data);

    NodeType type() const;
    size_t length() const;
    std::vector<std::string> keys() const;
    Node operator[](const std::string& key) const;
    Node operator[](size_t idx) const;
    explicit operator bool() const;

    bool operator>>(std::string& str) const;
    bool operator>>(int64_t& n) const;
    bool operator>>(double& n) const;
    bool operator>>(bool& b) const;

    std::string to_string() const;

    std::shared_ptr<const IStructuredData> node_;
  };

  using Dict = std::map<std::string, Node>;

  template <typename T>
  Node::Node(T&& implicitly_adaptable_data) : node_(as_structured_data(std::forward<T>(implicitly_adaptable_data))) {}

  template <>
  struct StructuredDataAdapter<Node> : IStructuredData {
    StructuredDataAdapter(Node node) : node_(std::move(node)) {}
    NodeType type() const {
      return node_.type();
    }
    size_t length() const {
      return node_.length();
    }
    std::vector<std::string> keys() const {
      return node_.keys();
    }
    std::shared_ptr<const IStructuredData>
    operator[](const std::string& str) const {
      return node_[str].node_;
    }
    std::shared_ptr<const IStructuredData>
    operator[](size_t idx) const {
      return node_[idx].node_;
    }
    Maybe<std::string>  get_string() const {
      std::string s;
      if (node_ >> s) return s;
      return Nothing;
    }
    Maybe<int64_t> get_integer() const {
      int64_t n;
      if (node_ >> n) return n;
      return Nothing;
    }
    Maybe<double>  get_float() const {
      double d;
      if (node_ >> d) return d;
      return Nothing;
    }
    Maybe<bool> get_boolean() const {
      bool b;
      if (node_ >> b) return b;
      return Nothing;
    }
  private:
    Node node_;
  };

  inline
  Node::Node(std::map<std::string, Node> dict) : node_(as_structured_data(std::move(dict))) {}
}

#endif // WAYWARD_SUPPORT_NODE_HPP_INCLUDED

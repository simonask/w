#pragma once
#ifndef WAYWARD_SUPPORT_NODE_HPP_INCLUDED
#define WAYWARD_SUPPORT_NODE_HPP_INCLUDED

#include <wayward/support/maybe.hpp>
#include <wayward/support/structured_data_adapters.hpp>
#include <wayward/support/error.hpp>

namespace wayward {
  /*
    A Node is a form of type-erasure interface that provides generic access, for instance to templating engines or serialization purposes.

    Think of it as any subtree of JSON data.
  */
  struct Node {
    explicit Node(StructuredDataConstPtr ptr) : ptr_(std::move(ptr)) {}
    Node(NothingType) {}
    Node() {}
    Node(const Node&) = default;
    Node(Node&&) = default;
    Node& operator=(const Node&) = default;
    Node& operator=(Node&&) = default;
    template <typename T>
    Node(T&& implicitly_adaptable_data);

    NodeType type() const;
    size_t length() const;
    std::vector<std::string> keys() const;
    Node get(const std::string& key) const;
    Node get(size_t idx) const;
    Node operator[](const std::string& key) const;
    Node operator[](size_t idx) const;
    explicit operator bool() const;

    // Extractors that try their hardest to convert the data to the target type:
    bool operator>>(std::string& str) const;
    bool operator>>(int64_t& n) const;
    bool operator>>(double& n) const;
    bool operator>>(bool& b) const;

    std::string to_string() const;

    StructuredDataConstPtr ptr_;
  };

  using Dict = std::map<std::string, Node>;

  template <> struct GetStructuredDataAdapter<Node> {
    static StructuredDataConstPtr get(const Node& node) {
      return node.ptr_;
    }
  };

  template <> struct GetStructuredDataAdapter<Node&> {
    static StructuredDataConstPtr get(const Node& node) {
      return node.ptr_;
    }
  };

  template <> struct GetStructuredDataAdapter<Node&&> {
    static StructuredDataConstPtr get(Node&& node) {
      return std::move(node.ptr_);
    }
  };

  template <typename T>
  Node::Node(T&& implicitly_adaptable_data)
  : ptr_(
    std::static_pointer_cast<const IStructuredData>(
      GetStructuredDataAdapter<T>::get(
        std::forward<T>(implicitly_adaptable_data)
      )
    )
  )
  {}
}

#endif // WAYWARD_SUPPORT_NODE_HPP_INCLUDED

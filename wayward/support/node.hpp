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
  private:
    std::shared_ptr<const IStructuredData> node_;
  };


  template <typename T>
  Node::Node(T&& implicitly_adaptable_data) : node_(as_structured_data(std::forward<T>(implicitly_adaptable_data))) {}
}

#endif // WAYWARD_SUPPORT_NODE_HPP_INCLUDED

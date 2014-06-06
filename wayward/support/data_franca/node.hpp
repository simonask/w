#pragma once
#ifndef WAYWARD_SUPPORT_DATA_FRANCA_NODE_HPP_INCLUDED
#define WAYWARD_SUPPORT_DATA_FRANCA_NODE_HPP_INCLUDED

#include <wayward/support/data_franca/types.hpp>

#include <wayward/support/cloning_ptr.hpp>
#include <wayward/support/either.hpp>

namespace wayward {
  namespace data_franca {
    struct Node {
      Node(const Node& other) = default;
      Node(Node&& other) = default;
      Node(NothingType = Nothing) : data_(Nothing) {}
      Node(Boolean b)  : data_(b) {}
      Node(Integer n)  : data_(n) {}
      Node(Real r)     : data_(r) {}
      Node(String str) : data_(std::move(str)) {}

      Node clone() const { return *this; }

      // Immutable interface:
      DataType type() const;
      bool is_nothing() const;
      operator bool() const;
      bool operator>>(Boolean& b) const;
      bool operator>>(Integer& n) const;
      bool operator>>(Real& r) const;
      bool operator>>(String& str) const;
      size_t length() const;
      const Node& operator[](size_t idx) const;
      const Node& operator[](const String& key) const;
      struct iterator;
      iterator begin() const;
      iterator end()   const;

      // Mutable interface -- changes the type of the node:
      Node& operator=(const Node& other) = default;
      Node& operator=(Node&& other) = default;
      Node& operator[](size_t idx);
      Node& operator[](const String& key);
      void push_back(Node value);

    private:
      using List = std::vector<CloningPtr<Node>>;
      using Dictionary = std::map<std::string, CloningPtr<Node>>;

      Either<
        NothingType,
        Boolean,
        Integer,
        Real,
        String,
        List,
        Dictionary
      > data_;

      static const Node forever_empty_;
    };
  }
}

#endif // WAYWARD_SUPPORT_DATA_FRANCA_NODE_HPP_INCLUDED

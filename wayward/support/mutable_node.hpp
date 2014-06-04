#pragma once
#ifndef WAYWARD_SUPPORT_MUTABLE_NODE_HPP_INCLUDED
#define WAYWARD_SUPPORT_MUTABLE_NODE_HPP_INCLUDED

#include <wayward/support/node.hpp>
#include <wayward/support/structured_data.hpp>
#include <wayward/support/either.hpp>

namespace wayward {
  struct MutableNode {
    struct Data;
    using DataPtr = std::shared_ptr<Data>;

    struct Data : IStructuredData {
      NodeType type() const final;

      size_t length() const final;
      std::vector<std::string> keys() const final;

      StructuredDataConstPtr get(const std::string& str) const final;
      StructuredDataConstPtr get(size_t idx) const final;
      Maybe<std::string> get_string() const final;
      Maybe<int64_t>     get_integer() const final;
      Maybe<double>      get_float() const final;
      Maybe<bool>        get_boolean() const final;

      using Dictionary = std::map<std::string, DataPtr>;
      using List = std::vector<DataPtr>;
      Either<NothingType, bool, int64_t, double, std::string, List, Dictionary> value_;

      Data() : value_{Nothing} {}
    };

    /// Immutable interface (should be equivalent to Node):

    NodeType type() const;
    size_t length() const;
    std::vector<std::string> keys() const;
    Node get(const std::string& key) const;
    Node get(size_t idx) const;
    Node operator[](const std::string& key) const;
    Node operator[](size_t idx) const;
    explicit operator bool() const;

    bool operator>>(std::string& str) const;
    bool operator>>(int64_t& n) const;
    bool operator>>(double& n) const;
    bool operator>>(bool& b) const;

    std::string to_string() const { return Node{*this}.to_string(); }

    operator Node() const { return Node{data()}; }

    /// Mutable interface:
    MutableNode(NothingType = Nothing) : data_{new Data} {}
    MutableNode(std::string str) : MutableNode() { data_->value_ = std::move(str); }
    MutableNode(const char* str) : MutableNode() { data_->value_ = std::string{str}; }
    MutableNode(int n)           : MutableNode() { data_->value_ = (int64_t)n; }
    MutableNode(int64_t n)       : MutableNode() { data_->value_ = n; }
    MutableNode(double d)        : MutableNode() { data_->value_ = d; }
    MutableNode(bool b)          : MutableNode() { data_->value_ = b; }
    static MutableNode dictionary() { MutableNode node; node.data_->value_ = Data::Dictionary{}; return std::move(node); }
    static MutableNode list()       { MutableNode node; node.data_->value_ = Data::List{};       return std::move(node); }

    // Copy, move:
    MutableNode(const MutableNode& other) : MutableNode() { data_->value_ = other.data_->value_; }
    MutableNode(MutableNode&& other) : data_(std::move(other.data_)) {}
    MutableNode& operator=(const MutableNode& other) { data_->value_ = other.data_->value_; return *this; }
    MutableNode& operator=(MutableNode&& other)      { data_->value_ = std::move(other.data_->value_); return *this; }

    // Coerces the type to Dictionary:
    MutableNode operator[](const std::string& key);

    // Coerces the type to List:
    void push_back(MutableNode node);

    DataPtr data() & { return data_; }
    DataPtr data() && { return std::move(data_); }
    std::shared_ptr<const IStructuredData> data() const& { return std::static_pointer_cast<const IStructuredData>(data_); }

    MutableNode(DataPtr data) : data_{std::move(data)} {}
    void take_data(DataPtr new_ptr) { data_ = std::move(new_ptr); }
  private:
    DataPtr data_;
  };

  template <> struct GetStructuredDataAdapter<const MutableNode&> {
    static StructuredDataConstPtr get(const MutableNode& node) {
      return node.data();
    }
  };
  template <> struct GetStructuredDataAdapter<MutableNode&&> {
    static StructuredDataConstPtr get(MutableNode&& node) {
      return std::static_pointer_cast<const IStructuredData>(node.data());
    }
  };
  template <> struct GetStructuredDataAdapter<MutableNode> {
    static StructuredDataConstPtr get(MutableNode node) {
      return std::static_pointer_cast<const IStructuredData>(std::move(node).data());
    }
  };
  template <> struct GetStructuredDataAdapter<MutableNode&> {
    static StructuredDataConstPtr get(MutableNode& node) {
      return std::static_pointer_cast<const IStructuredData>(node.data());
    }
  };
}

#endif // WAYWARD_SUPPORT_MUTABLE_NODE_HPP_INCLUDED

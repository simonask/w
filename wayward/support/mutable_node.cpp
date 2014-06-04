#include "wayward/support/mutable_node.hpp"

namespace wayward {
  NodeType MutableNode::Data::type() const {
    // XXX: A little bit fragile.
    switch (value_.which()) {
      case 0: return NodeType::Nil;
      case 1: return NodeType::Boolean;
      case 2: return NodeType::Integer;
      case 3: return NodeType::Float;
      case 4: return NodeType::String;
      case 5: return NodeType::List;
      case 6: return NodeType::Dictionary;
      default: assert(false); // Invalid which()
    }
  }

  size_t MutableNode::Data::length() const {
    size_t n = 0;
    value_.template when<List>([&](const List& list) {
      n = list.size();
    });
    return n;
  }

  std::vector<std::string> MutableNode::Data::keys() const {
    std::vector<std::string> kx;
    value_.template when<Dictionary>([&](const Dictionary& dict) {
      kx.reserve(dict.size());
      for (auto& pair: dict) {
        kx.push_back(pair.first);
      }
    });
    return std::move(kx);
  }

  StructuredDataConstPtr MutableNode::Data::get(const std::string& str) const {
    StructuredDataConstPtr ptr;
    value_.template when<Dictionary>([&](const Dictionary& dict) {
      auto it = dict.find(str);
      if (it != dict.end()) {
        ptr = std::static_pointer_cast<const IStructuredData>(it->second);
      }
    });
    return std::move(ptr);
  }

  StructuredDataConstPtr MutableNode::Data::get(size_t idx) const {
    StructuredDataConstPtr ptr;
    value_.template when<List>([&](const List& list) {
      if (idx < list.size()) {
        ptr = std::static_pointer_cast<const IStructuredData>(list.at(idx));
      }
    });
    return std::move(ptr);
  }

  Maybe<std::string> MutableNode::Data::get_string() const {
    Maybe<std::string> str;
    value_.template when<std::string>([&](const std::string& s) {
      str = s;
    });
    return std::move(str);
  }

  Maybe<int64_t> MutableNode::Data::get_integer() const {
   Maybe<int64_t> integer;
    value_.template when<int64_t>([&](int64_t s) {
      integer = s;
    });
    return integer;
  }

  Maybe<double> MutableNode::Data::get_float() const {
    Maybe<double> number;
    value_.template when<double>([&](double s) {
      number = s;
    });
    return number;
  }

  Maybe<bool> MutableNode::Data::get_boolean() const {
    Maybe<bool> boolean;
    value_.template when<bool>([&](bool b) {
      boolean = b;
    });
    return boolean;
  }

  NodeType MutableNode::type() const {
    return data_->type();
  }

  size_t MutableNode::length() const {
    return data_->length();
  }

  std::vector<std::string> MutableNode::keys() const {
    return data_->keys();
  }

  Node MutableNode::get(const std::string& key) const {
    return Node{data_->get(key)};
  }

  Node MutableNode::get(size_t idx) const {
    return Node{data_->get(idx)};
  }

  Node MutableNode::operator[](const std::string& key) const {
    return get(key);
  }

  Node MutableNode::operator[](size_t idx) const {
    return get(idx);
  }

  bool MutableNode::operator>>(std::string& str) const {
    return Node{*this} >> str;
  }

  bool MutableNode::operator>>(int64_t& n) const {
    return Node{*this} >> n;
  }

  bool MutableNode::operator>>(double& d) const {
    return Node{*this} >> d;
  }

  bool MutableNode::operator>>(bool& b) const {
    return Node{*this} >> b;
  }

  MutableNode MutableNode::operator[](const std::string& key) {
    if (type() != NodeType::Dictionary) {
      data_->value_ = Data::Dictionary{};
    }
    DataPtr ptr;
    data_->value_.template when<Data::Dictionary>([&](Data::Dictionary& dict) {
      auto it = dict.find(key);
      if (it != dict.end()) {
        ptr = it->second;
      } else {
        ptr = std::make_shared<Data>();
        dict[key] = ptr;
      }
    });
    if (ptr == nullptr) {
      ptr = std::make_shared<Data>();
    }
    return MutableNode{std::move(ptr)};
  }

  void MutableNode::push_back(MutableNode node) {
    if (type() != NodeType::List) {
      data_->value_ = Data::List{};
    }
    data_->value_.template when<Data::List>([&](Data::List& list) {
      list.push_back(std::move(std::move(node).data()));
    });
  }
}

#include <wayward/support/node.hpp>

namespace wayward {
  NodeType Node::type() const {
    return node_ ? node_->type() : NodeType::Nil;
  }

  size_t Node::length() const {
    return node_ ? node_->length() : 0;
  }

  std::vector<std::string> Node::keys() const {
    return node_ ? node_->keys() : std::vector<std::string>();
  }

  Node Node::operator[](const std::string& key) const {
    if (node_) {
      return Node((*node_)[key]);
    } else {
      return Node();
    }
  }

  Node Node::operator[](size_t idx) const {
    if (node_) {
      return Node((*node_)[idx]);
    } else {
      return Node();
    }
  }

  Node::operator bool() const {
    return node_ && node_->type() != NodeType::Nil;
  }

  bool Node::operator>>(std::string& str) const {
    if (node_) {
      auto m = node_->get_string();
      if (m) {
        str = *m;
        return true;
      }
    }
    return false;
  }

  bool Node::operator>>(std::int64_t& n) const {
    if (node_) {
      auto m = node_->get_integer();
      if (m) {
        n = *m;
        return true;
      }
    }
    return false;
  }

  bool Node::operator>>(double& n) const {
    if (node_) {
      auto m = node_->get_float();
      if (m) {
        n = *m;
        return true;
      }
    }
    return false;
  }
}

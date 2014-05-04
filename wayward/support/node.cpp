#include <wayward/support/node.hpp>

#include <stdlib.h> // For strtol, strtod
#include <errno.h>

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

  namespace {
    bool str2int64_base10(const std::string& input, int64_t& out) {
      if (input.size() == 0) {
        return false;
      }

      char* endp;
      errno = 0;
      out = ::strtoll(input.c_str(), &endp, 10);
      if (errno != 0) {
        return false;
      }
      if (endp == input.c_str() + input.size()) {
        return true;
      }
      return false;
    }

    bool str2double(const std::string& input, double& out) {
      if (input.size() == 0) {
        return false;
      }

      char* endp;
      errno = 0;
      out = ::strtod(input.c_str(), &endp);
      if (errno != 0) {
        return false;
      }
      if (endp == input.c_str() + input.size()) {
        return true;
      }
      return false;
    }
  }

  bool Node::operator>>(std::string& str) const {
    if (node_) {
      switch (type()) {
        case NodeType::Integer: {
          std::stringstream ss;
          ss << *node_->get_integer();
          str = ss.str();
          return true;
        }
        case NodeType::Float: {
          std::stringstream ss;
          ss << *node_->get_float();
          str = ss.str();
          return true;
        }
        case NodeType::String: {
          str = *node_->get_string();
          return true;
        }
        default:
          return false;
      }
    }
    return false;
  }

  bool Node::operator>>(std::int64_t& n) const {
    if (node_) {
      switch (type()) {
        case NodeType::Integer: {
          n = *node_->get_integer();
          return true;
        }
        case NodeType::Float: {
          n = *node_->get_float();
          return true;
        }
        case NodeType::String: {
          return str2int64_base10(*node_->get_string(), n);
        }
        default:
          return false;
      }
    }
    return false;
  }

  bool Node::operator>>(double& n) const {
    if (node_) {
      switch (type()) {
        case NodeType::Integer: {
          n = *node_->get_integer();
          return true;
        }
        case NodeType::Float: {
          n = *node_->get_float();
          return true;
        }
        case NodeType::String: {
          return str2double(*node_->get_string(), n);
        }
        default:
          return false;
      }
    }
    return false;
  }
}

#include <wayward/support/node.hpp>

#include <stdlib.h> // For strtol, strtod
#include <errno.h>

namespace wayward {
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

    template <class Ptr>
    bool coerce_scalar_node_to_string(const Ptr& ptr, std::string& str) {
      if (ptr) {
        switch (ptr->type()) {
          case NodeType::Integer: {
            std::stringstream ss;
            ss << *ptr->get_integer();
            str = ss.str();
            return true;
          }
          case NodeType::Float: {
            std::stringstream ss;
            ss << *ptr->get_float();
            str = ss.str();
            return true;
          }
          case NodeType::String: {
            str = *ptr->get_string();
            return true;
          }
          default: return false;
        }
      }
      return false;
    }

    template <class Ptr>
    bool coerce_scalar_node_to_integer(const Ptr& ptr, int64_t& n) {
      if (ptr) {
        switch (ptr->type()) {
          case NodeType::Integer: {
            n = *ptr->get_integer();
            return true;
          }
          case NodeType::Float: {
            n = *ptr->get_float();
            return true;
          }
          case NodeType::String: {
            return str2int64_base10(*ptr->get_string(), n);
          }
          default: return false;
        }
      }
      return false;
    }

    template <class Ptr>
    bool coerce_scalar_node_to_float(const Ptr& ptr, double& n) {
      if (ptr) {
        switch (ptr->type()) {
          case NodeType::Integer: {
            n = *ptr->get_integer();
            return true;
          }
          case NodeType::Float: {
            n = *ptr->get_float();
            return true;
          }
          case NodeType::String: {
            return str2double(*ptr->get_string(), n);
          }
          default:
            return false;
        }
      }
      return false;
    }

    template <class Ptr>
    bool coerce_scalar_node_to_boolean(const Ptr& ptr, bool& b) {
      if (ptr) {
        switch (ptr->type()) {
          case NodeType::Boolean: {
            b = *ptr->get_boolean();
            return true;
          }
          case NodeType::String: {
            std::string str = *ptr->get_string();
            if (str == "true") {
              b = true;
              return true;
            } else if (str == "false") {
              b = false;
              return true;
            } else {
              return false;
            }
          }
          default: return false;
        }
      }
      return false;
    }

    std::string node_to_string_representation(const Node& node) {
      switch (node.type()) {
        case NodeType::Nil: {
          return "nil";
        }
        case NodeType::Boolean: {
          bool b = false;
          node >> b;
          return b ? "true" : "false";
        }
        case NodeType::Integer: {
          std::string str;
          node >> str;
          return std::move(str);
        }
        case NodeType::Float: {
          std::string str;
          node >> str;
          return std::move(str);
        }
        case NodeType::String: {
          std::string str;
          node >> str;
          return str;
        }
        case NodeType::List: {
          std::stringstream ss;
          ss << '[';
          for (size_t i = 0; i < node.length(); ++i) {
            auto n = node[i];
            if (n.type() == NodeType::String) {
              ss << "\"";
              ss << n.to_string();
              ss << "\"";
            } else {
              ss << n.to_string();
            }
            if (i + 1 != node.length()) {
              ss << ", ";
            }
          }
          ss << ']';
          return ss.str();
        }
        case NodeType::Dictionary: {
          std::stringstream ss;
          ss << '{';
          auto kx = node.keys();
          for (size_t i = 0; i < kx.size(); ++i) {
            auto n = node[kx[i]];
            ss << kx[i] << ": ";
            if (n.type() == NodeType::String) {
              ss << "\"";
              ss << n.to_string();
              ss << "\"";
            } else {
              ss << n.to_string();
            }
            if (i + 1 != kx.size()) {
              ss << ", ";
            }
          }
          ss << '}';
          return ss.str();
        }
      }
    }

    bool is_node_truthy(const Node& node) {
      auto t = node.type();
      if (t == NodeType::Nil)
        return false;
      if (t == NodeType::Boolean)
        return *node.ptr_->get_boolean();
      return true;
    }
  }

  NodeType Node::type() const {
    return ptr_ ? ptr_->type() : NodeType::Nil;
  }

  size_t Node::length() const {
    return ptr_ ? ptr_->length() : 0;
  }

  std::vector<std::string> Node::keys() const {
    return ptr_ ? ptr_->keys() : std::vector<std::string>{};
  }

  Node Node::operator[](const std::string& key) const {
    return ptr_ ? Node{ptr_->get(key)} : Node{};
  }

  Node Node::operator[](size_t idx) const {
    return ptr_ ? Node{ptr_->get(idx)} : Node{};
  }

  Node::operator bool() const {
    return is_node_truthy(*this);
  }

  bool Node::operator>>(std::string& str) const {
    return coerce_scalar_node_to_string(ptr_, str);
  }

  bool Node::operator>>(std::int64_t& n) const {
    return coerce_scalar_node_to_integer(ptr_, n);
  }

  bool Node::operator>>(double& n) const {
    return coerce_scalar_node_to_float(ptr_, n);
  }

  bool Node::operator>>(bool& b) const {
    return coerce_scalar_node_to_boolean(ptr_, b);
  }

  std::string Node::to_string() const {
    return node_to_string_representation(*this);
  }
}

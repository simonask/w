#include <wayward/support/json.hpp>
#include <sstream>

namespace wayward {
  namespace {
    void escape_json_stream(std::ostream& os, const std::string& input) {
      for (auto it = input.cbegin(); it != input.cend(); it++) {
        switch (*it) {
            case '\\': os << "\\\\"; break;
            case '"':  os << "\\\""; break;
            case '/':  os << "\\/";  break;
            case '\b': os << "\\b";  break;
            case '\f': os << "\\f";  break;
            case '\n': os << "\\n";  break;
            case '\r': os << "\\r";  break;
            case '\t': os << "\\t";  break;
            default:   os << *it;  break;
        }
      }
    }

    void indentation(std::ostream& os, int indent = 0) {
      for (int i = 0; i < indent; ++i) {
        os << "  ";
      }
    }

    void node_to_json_stream(std::ostream& os, const Node& node, JSONMode mode, int indent = 0) {
      switch (node.type()) {
        case NodeType::Nil: {
          os << "null";
          break;
        }
        case NodeType::Integer: {
          int64_t n;
          if (node >> n) {
            os << n;
          } else {
            os << "undefined";
            throw JSONSerializationError("Integer node didn't return an integer.");
          }
          break;
        }
        case NodeType::Float: {
          double n;
          if (node >> n) {
            os << n;
          } else {
            os << "undefined";
            throw JSONSerializationError("Float node didn't return a double.");
          }
          break;
        }
        case NodeType::String: {
          std::string str;
          if (node >> str) {
            os << "\"";
            escape_json_stream(os, str);
            os << "\"";
          } else {
            os << "undefined";
            throw JSONSerializationError("String node didn't return a string.");
          }
          break;
        }
        case NodeType::List: {
          size_t len = node.length();
          os << '[';
          if (mode == JSONMode::Compact) {
            for (size_t i = 0; i < len; ++i) {
              node_to_json_stream(os, node[i], mode);
              if (i+1 != len) {
                os << ", ";
              }
            }
          } else {
            if (len != 0) {
              os << '\n';
              indentation(os, indent);
              for (size_t i = 0; i < len; ++i) {
                indentation(os, 1);
                node_to_json_stream(os, node[i], mode, indent+1);
                if (i+1 != len) {
                  os << ',';
                }
                os << '\n';
                indentation(os, indent);
              }
            }
          }
          os << ']';
          break;
        }
        case NodeType::Dictionary: {
          auto keys = node.keys();
          size_t len = keys.size();
          os << '{';
          if (mode == JSONMode::Compact) {
            for (auto it = keys.begin(); it != keys.end(); ++it) {
              os << "\"";
              escape_json_stream(os, *it);
              os << "\": ";
              node_to_json_stream(os, node[*it], mode);
              if (it+1 != keys.end()) {
                os << ", ";
              }
            }
          } else {
            if (len != 0) {
              indentation(os, indent);
              for (auto it = keys.begin(); it != keys.end(); ++it) {
                indentation(os, 1);
                os << "\"";
                escape_json_stream(os, *it);
                os << "\": ";
                node_to_json_stream(os, node[*it], mode, indent+1);
                if (it+1 != keys.end()) {
                  os << ',';
                }
                os << '\n';
                indentation(os, indent);
              }
            }
          }
          os << '}';
          break;
        }
      }
    }
  }

  std::string escape_json(const std::string& input) {
    std::stringstream ss;
    escape_json_stream(ss, input);
    return ss.str();
  }

  std::string as_json(const Node& node, JSONMode mode) {
    std::stringstream ss;
    node_to_json_stream(ss, node, mode);
    return ss.str();
  }
}

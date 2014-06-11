#include <wayward/support/json.hpp>
#include <sstream>

namespace wayward {
  namespace {
    using namespace data_franca;

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

    void node_to_json_stream(std::ostream& os, const Spectator& node, JSONMode mode, int indent = 0) {
      switch (node.type()) {
        case DataType::Nothing: {
          os << "null";
          break;
        }
        case DataType::Boolean: {
          bool b;
          if (node >> b) {
            os << (b ? "true" : "false");
          } else {
            os << "undefined";
            throw JSONSerializationError("Boolean node didn't return a bool");
          }
          break;
        }
        case DataType::Integer: {
          Integer n;
          if (node >> n) {
            os << n;
          } else {
            os << "undefined";
            throw JSONSerializationError("Integer node didn't return an integer.");
          }
          break;
        }
        case DataType::Real: {
          Real n;
          if (node >> n) {
            os << n;
          } else {
            os << "undefined";
            throw JSONSerializationError("Float node didn't return a double.");
          }
          break;
        }
        case DataType::String: {
          String str;
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
        case DataType::List: {
          size_t len = node.length();
          os << '[';
          if (mode == JSONMode::Compact) {
            auto end = node.end();
            for (auto it = node.begin(); it != end;) {
              node_to_json_stream(os, *it, mode);
              ++it;
              if (it != end) {
                os << ", ";
              }
            }
          } else {
            if (len != 0) {
              os << '\n';
              indentation(os, indent);
              auto end = node.end();
              for (auto it = node.begin(); it != end;) {
                indentation(os, 1);
                node_to_json_stream(os, *it, mode, indent+1);
                ++it;
                if (it != end) {
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
        case DataType::Dictionary: {
          os << '{';
          if (mode == JSONMode::Compact) {
            auto end = node.end();
            for (auto it = node.begin(); it != end;) {
              os << "\"";
              escape_json_stream(os, *it.key());
              os << "\": ";
              node_to_json_stream(os, *it, mode);
              ++it;
              if (it != end) {
                os << ", ";
              }
            }
          } else {
            if (node.length() != 0) {
              indentation(os, indent);
              auto end = node.end();
              for (auto it = node.begin(); it != end;) {
                indentation(os, 1);
                escape_json_stream(os, *it.key());
                os << "\": ";
                node_to_json_stream(os, *it, mode, indent+1);
                ++it;
                if (it != end) {
                  os << ", ";
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

  std::string as_json(const data_franca::Spectator& node, JSONMode mode) {
    std::stringstream ss;
    node_to_json_stream(ss, node, mode);
    return ss.str();
  }
}

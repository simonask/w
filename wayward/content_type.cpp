#include "wayward/content_type.hpp"

#include <map>

namespace wayward {
  constexpr char HTML::MimeType[];
  constexpr char JSON::MimeType[];
  constexpr char XML::MimeType[];

  namespace {
    struct ContentTypeRegistry {
      std::map<std::string, std::string> content_types;
    };

    ContentTypeRegistry& content_type_registry() {
      static ContentTypeRegistry registry;
      return registry;
    }

    struct RegisterDefaultContentTypeExtensions {
      RegisterDefaultContentTypeExtensions() {
        register_content_type_extension<HTML>("html");
        register_content_type_extension<HTML>("htm");
        register_content_type_extension<XML>("xml");
        register_content_type_extension<JSON>("json");
      }
    };

    static const RegisterDefaultContentTypeExtensions _g_register_default_content_type_extensions = RegisterDefaultContentTypeExtensions{};
  }

  void register_content_type_extension(std::string ext, std::string content_type) {
    content_type_registry().content_types[std::move(ext)] = std::move(content_type);
  }

  Maybe<std::string> content_type_for_extension(std::string ext) {
    auto it = content_type_registry().content_types.find(ext);
    if (it != content_type_registry().content_types.end()) {
      return it->second;
    }
    return Nothing;
  }
}

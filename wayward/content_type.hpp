#pragma once
#ifndef WAYWARD_CONTENT_TYPE_HPP_INCLUDED
#define WAYWARD_CONTENT_TYPE_HPP_INCLUDED

#include <wayward/support/maybe.hpp>

namespace wayward {
  struct HTML {
    static const constexpr char MimeType[] = "text/html";
  };

  struct JSON {
    static const constexpr char MimeType[] = "application/json";
  };

  struct XML {
    static const constexpr char MimeType[] = "application/xml";
  };

  Maybe<std::string> content_type_for_extension(std::string ext);
  void register_content_type_extension(std::string ext, std::string content_type);

  template <typename ContentType>
  void register_content_type_extension(std::string ext) {
    register_content_type_extension(std::move(ext), ContentType::MimeType);
  }
}

#endif // WAYWARD_CONTENT_TYPE_HPP_INCLUDED

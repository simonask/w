#pragma once
#ifndef WAYWARD_CONTENT_TYPE_HPP_INCLUDED
#define WAYWARD_CONTENT_TYPE_HPP_INCLUDED

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
}

#endif // WAYWARD_CONTENT_TYPE_HPP_INCLUDED

#ifndef W_URI_HPP_INCLUDED
#define W_URI_HPP_INCLUDED

#include <string>
#include <wayward/support/maybe.hpp>

namespace wayward {
  struct URI {
    URI() {}
    URI(std::string scheme, std::string host, int port, std::string path, std::string query = "", std::string fragment = "");
    URI(URI&& other) = default;
    URI(const URI& other) = default;
    URI& operator=(URI&& other) = default;
    URI& operator=(const URI& other) = default;

    static Maybe<URI> parse(const std::string& uri);

    static std::string decode(const std::string& encoded);
    static std::string encode(const std::string& raw);

    std::string scheme;
    std::string username;
    std::string password;
    std::string host;
    int port;
    std::string path;
    std::string query;
    std::string fragment;

    std::string to_string() const;
  };

  inline URI::URI(std::string scheme, std::string host, int port, std::string path, std::string query, std::string fragment) : scheme(std::move(scheme)), host(std::move(host)), port(port), path(std::move(path)), query(std::move(query)), fragment(std::move(fragment)) {}

  template <typename OS>
  OS& operator<<(OS& os, const URI& uri) {
    os << uri.to_string();
    return os;
  }
}

#endif /* end of include guard: SYMBOL */

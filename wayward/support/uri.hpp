#ifndef W_URI_HPP_INCLUDED
#define W_URI_HPP_INCLUDED

#include <string>

namespace wayward {
  struct URI {
    URI() {}
    explicit URI(std::string uri);
    URI(std::string scheme, std::string host, int port, std::string path, std::string query = "", std::string fragment = "");
    URI(URI&& other) = default;
    URI(const URI& other) = default;
    URI& operator=(URI&& other) = default;
    URI& operator=(const URI& other) = default;

    std::string scheme;
    std::string host;
    int port;
    std::string path;
    std::string query;
    std::string fragment;
  };

  inline URI::URI(std::string scheme, std::string host, int port, std::string path, std::string query, std::string fragment) : scheme(std::move(scheme)), host(std::move(host)), port(port), path(std::move(path)), query(std::move(query)), fragment(std::move(fragment)) {}
}

#endif /* end of include guard: SYMBOL */

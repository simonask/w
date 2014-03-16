#include "wayward/support/uri.hpp"
#include <event2/http.h>

namespace w {
  #define P ((evhttp_uri*)priv)

  URI::URI() {
    priv = evhttp_uri_new();
  }

  URI::URI(std::string uri) {
    priv = evhttp_uri_parse(uri.c_str());
  }

  URI::URI(std::string scheme, std::string host, std::string path, std::string query, std::string fragment) {
    priv = evhttp_uri_new();
    evhttp_uri_set_scheme(P, scheme.c_str());
    evhttp_uri_set_host(P, host.c_str());
    evhttp_uri_set_path(P, path.c_str());
    evhttp_uri_set_query(P, path.c_str());
    evhttp_uri_set_fragment(P, fragment.c_str());
  }

  URI::URI(URI&& other) {
    std::swap(priv, other.priv);
  }

  URI::URI(const URI& other) {
    priv = evhttp_uri_new();
    evhttp_uri_set_scheme(P, evhttp_uri_get_scheme((evhttp_uri*)other.priv));
    evhttp_uri_set_host(P, evhttp_uri_get_host((evhttp_uri*)other.priv));
    evhttp_uri_set_path(P, evhttp_uri_get_path((evhttp_uri*)other.priv));
    evhttp_uri_set_query(P, evhttp_uri_get_path((evhttp_uri*)other.priv));
    evhttp_uri_set_fragment(P, evhttp_uri_get_fragment((evhttp_uri*)other.priv));
  }

  URI& URI::operator=(URI&& other) {
    std::swap(priv, other.priv);
    return *this;
  }

  URI& URI::operator=(const URI& other) {
    evhttp_uri_set_scheme(P, evhttp_uri_get_scheme((evhttp_uri*)other.priv));
    evhttp_uri_set_host(P, evhttp_uri_get_host((evhttp_uri*)other.priv));
    evhttp_uri_set_path(P, evhttp_uri_get_path((evhttp_uri*)other.priv));
    evhttp_uri_set_query(P, evhttp_uri_get_path((evhttp_uri*)other.priv));
    evhttp_uri_set_fragment(P, evhttp_uri_get_fragment((evhttp_uri*)other.priv));
    return *this;
  }


  URI::~URI() {
    if (P)
      evhttp_uri_free(P);
  }

  std::string URI::scheme() const {
    return evhttp_uri_get_scheme(P);
  }
  std::string URI::host() const {
    return evhttp_uri_get_host(P);
  }
  int URI::port() const {
    return evhttp_uri_get_port(P);
  }
  std::string URI::path() const {
    return evhttp_uri_get_path(P);
  }
  std::string URI::query() const {
    return evhttp_uri_get_query(P);
  }
  std::string URI::fragment() const {
    return evhttp_uri_get_fragment(P);
  }
}

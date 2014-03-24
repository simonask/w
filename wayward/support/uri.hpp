#ifndef W_URI_HPP_INCLUDED
#define W_URI_HPP_INCLUDED

#include <string>

namespace wayward {
  struct URI {
    URI();
    explicit URI(std::string uri);
    URI(std::string scheme, std::string host, std::string path, std::string query = "", std::string fragment = "");
    URI(URI&& other);
    URI(const URI& other);
    URI& operator=(URI&& other);
    URI& operator=(const URI& other);
    ~URI();

    std::string scheme() const;
    std::string host() const;
    int port() const;
    std::string path() const;
    std::string query() const;
    std::string fragment() const;
  private:
    void* priv = nullptr;
  };
}

#endif /* end of include guard: SYMBOL */

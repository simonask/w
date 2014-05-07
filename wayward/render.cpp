#include "wayward/w.hpp"

#include <fstream>

namespace wayward {
  Response redirect(std::string new_location, HTTPStatusCode code) {
    Response r;
    r.code = code;
    r.headers["Location"] = new_location;
    return r;
  }

  Response file(std::string path, Maybe<std::string> content_type) {
    std::ifstream fs(path);
    if (fs.good()) {
      fs.seekg(0, std::ios::end);
      auto len = fs.tellg();
      fs.seekg(0, std::ios::beg);
      std::string buffer;
      buffer.resize(len);
      fs.read(&buffer[0], len);

      Response response;
      response.body = std::move(buffer);
      if (content_type) {
        response.headers["Content-Type"] = *content_type;
      }
      return std::move(response);
    } else {
      return wayward::not_found();
    }
  }
}

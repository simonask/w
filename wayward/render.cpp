#include "wayward/w.hpp"
#include "wayward/template_engine.hpp"

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

  Response render(const std::string& template_name, Options params, HTTPStatusCode code) {
    auto engine = current_template_engine();
    Response response;
    response.code = code;
    response.body = engine->render(template_name, std::move(params));
    response.headers["Content-Type"] = "text/html; charset=utf-8";
    return std::move(response);
  }
}

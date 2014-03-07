#include "w.hpp"

namespace w {
  Response render_text(std::string text) {
    Response r;
    r.code = HTTPStatusCode::OK;
    r.headers["Content-Type"] = "text/plain";
    r.body = std::move(text);
    return r;
  }

  Response redirect(std::string new_location, HTTPStatusCode code) {
    Response r;
    r.code = code;
    r.headers["Location"] = new_location;
    return r;
  }
}

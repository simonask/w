#include "w.hpp"

namespace w {
  Response render_text(std::string text) {
    Response r;
    r.code = HTTPStatusCode::OK;
    r.headers["Content-Type"] = "text/plain";
    r.body = std::move(text);
    return r;
  }
}

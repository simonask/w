#include "wayward/w.hpp"

namespace wayward {
  Response redirect(std::string new_location, HTTPStatusCode code) {
    Response r;
    r.code = code;
    r.headers["Location"] = new_location;
    return r;
  }
}

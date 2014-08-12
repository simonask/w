#include "wayward/respond_to.hpp"

namespace wayward {
  Responder::Responder(Request&  req) : request_(req) {}
  Responder::Responder(Request&& req) : owned_request_(std::move(req)), request_(*owned_request_.unsafe_get()) {}
  Responder::Responder(Request&  req, std::string default_content_type) : request_(req), default_content_type_(std::move(default_content_type)) {}
  Responder::Responder(Request&& req, std::string default_content_type) : owned_request_(std::move(req)), request_(*owned_request_.unsafe_get()), default_content_type_(std::move(default_content_type)) {}

  Response Responder::to_response() const {
    auto accept_header_it = req_.headers.find("Accept");
    std::vector<std::string> accepted_content_types;
    if (accept_header_it) {
      accepted_content_types = split(*accept_header_it, ";");
    } else if (default_content_type_) {
      accepted_content_types = { *default_content_type_ };
    }


  }
}

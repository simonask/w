#include "wayward/respond_to.hpp"
#include "wayward/support/string.hpp"

namespace wayward {
  namespace {
    std::set<std::string> get_content_types(const Request& request) {
      std::set<std::string> result;
      std::string param_format;
      if (request.params["format"] >> param_format) {
        auto maybe_content_type = content_type_for_extension(param_format);
        monad::fmap(std::move(maybe_content_type), [&](auto& content_type) {
          return result = {std::move(content_type)};
        });
      }

      auto it = request.headers.find("Accept");
      if (it != request.headers.end()) {
        auto types = split(it->second, ";");
        for (auto& t: types) {
          auto trimmed = trim(t);
          if (t == "*/*") return std::move(result);
          result.insert(std::move(trimmed));
        }
      }
      return std::move(result);
    }

    Response not_acceptable() {
      Response response;
      response.code = HTTPStatusCode::NotAcceptable;
      response.headers["Content-Type"] = "text/plain";
      response.body = "Not Acceptable";
      return std::move(response);
    }
  }

  Responder::Responder(const Request& req)
  : request_(req)
  , accepted_content_types_(get_content_types(req))
  {}

  Responder::Responder(const Request& req, std::string default_content_type)
  : request_(req)
  , accepted_content_types_(get_content_types(req))
  , default_content_type_(std::move(default_content_type))
  {}

  Response Responder::to_response() const& {
    if (generated_response_) {
      return *generated_response_;
    } else {
      return not_acceptable();
    }
  }

  Response Responder::to_response() && {
    if (generated_response_) {
      return std::move(*generated_response_);
    } else {
      return not_acceptable();
    }
  }

  bool Responder::should_accept_content_type(const std::string& content_type) const {
    if (!generated_response_) {
      if (default_content_type_ && *default_content_type_ == content_type)
        return true;
      if (accepted_content_types_.size() == 0)
        return true;
      if (accepted_content_types_.count(content_type) == 1)
        return true;
    }
    return false;
  }
}

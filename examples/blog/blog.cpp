#include <w>
#include <wayward/support/json.hpp>
#include "models.hpp"

namespace app {
  template <typename T>
  w::Response json_response(const w::Request& req, T&& object, w::HTTPStatusCode code = w::HTTPStatusCode::OK) {
    w::Response response;
    // TODO: Set headers from req
    response.body = w::as_json(std::forward<T>(object));
    response.code = code;
    return response;
  }

  template <typename T>
  w::Response json_ok(const w::Request& req, T&& object) {
    return json_response(req, std::forward<T>(object));
  }

  w::Response index(w::Request& req) {
    p::Context ctx;
    auto posts = p::from<Post>(ctx)
      .where(p::column(&Post::published_at) <= DateTime::now())
      .inner_join(&Post::author)
      .order(&Post::published_at)
      .reverse_order();
    auto all = posts.all();

    return json_ok(req, all);
  }

  w::Response get_post(w::Request& req) {
    p::Context ctx;
    int64_t id;

    if (req.params["id"] >> id) {
      auto post = p::from<Post>(ctx).where(p::column(&Post::id) == id).first();
      if (post) {
        return json_response(req, post);
      }
    }

    return w::not_found();
  }
}

int main(int argc, char const *argv[])
{
  p::setup("postgresql://wayward_examples@localhost/wayward_examples_blog");

  w::App app;
  app.get("/", app::index);
  app.get("/posts/:id", app::get_post);

  return app.listen_and_serve("0.0.0.0", 3000);
}

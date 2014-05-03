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

  w::Response index(w::Request& req) {
    p::Context ctx;
    auto posts = p::from<Post>(ctx)
      .where(p::column(&Post::published_at) <= DateTime::now())
      .inner_join(&Post::author)
      .order(&Post::published_at)
      .reverse_order();
    auto all = posts.all();
    return json_response(req, all);
  }

  w::Response get_post(w::Request& req) {
    w::fail<w::Error>("Hello, errors!");
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

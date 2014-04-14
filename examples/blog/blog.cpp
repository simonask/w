#include <w>
#include "models.hpp"

namespace app {
  w::Request index(w::Request& req) {
    auto posts = p::from<Post>().where(p::column(&Post::published_at) <= p::sql("localtime")).order(&Post::published_at).reverse_order();
  }
}

int main(int argc, char const *argv[])
{
  p::setup("postgresql://wayward_examples_blog");

  w::App app;
  app.get("/", app::index);
  app.get("/posts/:id", app:get_post);

  return 0;
}

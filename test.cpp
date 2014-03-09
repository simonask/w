#include <w>
#include "format.hpp"
#include <iostream>

int main (int argc, char const *argv[])
{
  w::App app;

  app.get("/", [](w::Request& req) -> w::Response {
    return w::render_text("Hello, World!");
  });

  app.get("/articles/:id", [](w::Request& req) -> w::Response {
    return w::render_text("Hello, article 0!");
    // return w::render_template("article", req);
    // return w::render_template("article", {{"id", 2}, {"title", "Hejsa"}});
  });

  app.get("/articles/redirect", [](w::Request& req) -> w::Response {
    return w::redirect("/");
  });

  app.post("/articles", [](w::Request& req) -> w::Response {
    return w::Response();
  });

  std::cout << w::format("Hello, World! article_id = {id}\n", {{"id", 123}});
  std::cout << w::format("Hello first argument: {0} {1} {0}\n", 1, 2);

  return app.listen_and_serve("0.0.0.0", 3000);
}

#include <w>

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

  return app.listen_and_serve("0.0.0.0", 3000);
}

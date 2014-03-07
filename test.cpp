#include "w.hpp"

int main (int argc, char const *argv[])
{
  w::App app;

  app.get("/", [](w::Request& req) -> w::Response {
    return w::render_text("Hello, World!");
  });

  app.get("/articles/:id", [](w::Request& req) -> w::Response {
    return w::render_text("Hello, Articles!");
  });

	return app.listen_and_serve("0.0.0.0", 3000);
}

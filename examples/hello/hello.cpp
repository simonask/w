#include <w>

int main(int argc, char const *argv[])
{
  w::App app { argc, argv };

  app.get("/hello", [](w::Request&) { return w::render_text("Hello world"); });

  return app.run();
}

# Wayward Web Framework

Wayward lives in the namespace `wayward`, which by default is aliased as `w`. To disable this, compile your app with `WAYWARD_NO_SHORTHAND_NAMESPACE` defined. Every time you see `w::`, it can be substituted for `wayward::`.

The core of a Wayward app is an instance of `App`, which is used to define the routes and bring up a listening HTTP server.

The simplest Wayward app looks like this:

    #include <w>

    int main(int argc, char** argv) {
      w::App app { argc, argv };

      app.get("/", [](w::Request& req) {
        return w::render_text("Hello, World!");
      });

      return app.run();
    }

A slightly more advanced example could include a parameter in the route definition:

    #include <w>

    int main(int argc, char** argv) {
      w::App app { argc, argv };

      app.get("/hello/:person", [](w::Request& req) {
        return w::render_text("Hello, {0}!", req.params["person"]);
      });
    }

Going to `/hello/Steve` will then render the text "Hello, Steve!" in the browser.

See the documentation for [Routing](routing.md) for more ways to define routes.

See the [Advanced Routes](advanced_routes.md) for ways to define more complex route handlers.

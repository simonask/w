# Routing

Routes in Wayward are essentially functions that take a HTTP request as input and return a HTTP response.

The basic way to define a route is to simply call app.get/post/put/etc. with the desired path and a function object that can respond to request matching the path.

The `App` class does the heavy lifting in routes, and has the following interface:


---

# App

---

[wayward/w.hpp](https://github.com/simonask/w/blob/master/wayward/w.hpp)

## assets

Invoke: `assets(uri_path, filesystem_path)`

Define how and where static assets are located and accessed.

## get, put, patch, post, del, head, options

Invoke: `method(route, handler)`

Handler is a function with the signature `Response(Request&)`.

Route is a slash-separated path. Path elements beginning with `:` will be interpreted as input parameters to the request handler. Example:

    app.post("/foo/:bar", handler)

This route will respond to any path starting with "/foo/", and whatever comes after that will be passed to the handler in `request.params["bar"]`.

The method `del` defines a route that responds to DELETE requests, and is abbreviated to avoid collision with the C++ keyword `delete`.

## run

Invoke: `run()`

Starts listening and serving requests.

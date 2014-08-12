# Requests

All route handlers get called with a reference to a `w::Request` object. The `Request` object is a simple structure representing an HTTP request.

See also: [Wayward Support: HTTP Library](support/http.md)

From the perspective of a route handler, the most interesting parts of a request are the headers, the params, the [URI](support/uri.md), and the request body, if one is provided.

Params defined in the route with the `:` syntax will appear in the `params` member, as well as any GET and POST parameters passed from the client.

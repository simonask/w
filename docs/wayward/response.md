# Rendering Responses

Wayward comes with a number of predefined ways to render a quick HTTP response.

## w::not_found

Returns a generic 404 Not Found with Content-Type `text/plain`.

## w::render_text

Invoke: `render_text(format, ...)`

Returns a 200 OK response with Content-Type `text/plain`. The body of the response is the interpolation of the arguments. `w::render_text` treats its arguments equivalent to [`w::format`](support/format.md).

## w::redirect

Invoke: `redirect(new_url, [status])`

Returns a 302 Found response (or a different status code, if `status` is set), with the `Location` header set to `new_url`.

## w::render

See: [Templates](templates.md)

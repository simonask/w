TODO
====

Persistence
-----------

- INSERT
- UPDATE
- DELETE
- Migrations.
- Use asynchronous libpq API when running under libevent.

Wayward
-------

- Serving static files.
- Params de-/restructuralization
- Dictionary class that returns Maybe<T> for non-existing values.
- Templating system (use Synth?).

Wayward Support
---------------

- *Maybe* a better streaming API than <iostream>?
- *Maybe* an abstraction over the libevent runloop? For scheduling background tasks and such.
- Path utility functions (maybe a path class).
- A string class that does proper UTF-8 handling.
- A HTML class representing a string that is unescaped HTML.
- A green thread library, perhaps? GNU Pth?
- A 'reactor' pattern using threads for fibers.
- A HTTP library for making requests with libevent and green threads.

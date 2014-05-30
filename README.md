Wayward Web Framework
======================

Did your dad ever tell you: "Writing web apps in C++ is dangerous and stupid! You should know better!"?

Well he's not the boss of you! You can do whatever you want!

Wayward ("`<w>`") is a web framework for writing HTTP-aware apps in C++. It uses [libevent](http://libevent.org/) behind
the scenes to create a compact HTTP server that can act as the backend for your app.

`<w>` is heavily inspired by the Ruby framework Sinatra, and aims to keep things as simple as possible.
It uses the C++ Standard Template Library extensively.


But seriously, isn't this a bad idea?
-------------------------------------

Pretty much. C++ is really difficult to get right. However, most of the pitfalls come from
trying to be "too clever" and using advanced library-author features when they're
not necessary. Application control code in C++ is equally plain and boring as in
any other language, if done almost right, but many orders of magnitude faster.

Here are a couple of hints:

- Don't ever use a raw pointer for *anything* in your <w> app. You are most
  likely looking for an `std::vector`, an `std::string`, an `std::unique_ptr`, or an `std::shared_ptr`.
- If you find yourself implementing copy-constructors and move-constructors, you are likely
  creating bugs. See if you can represent your value type with just other value types that
  take care of resource management for you.
- Beware of `std::vector::operator[]`. It doesn't do bounds checking. Use `std::vector::at` instead.
- `<w>` may choose to handle more than one request at a time. If your request handlers share
  any state, make sure to guard it behind an `std::mutex`.

Features
--------

- An easy to use, high-level interface for defining HTTP-aware apps, similar to Ruby's Sinatra.
- Quick development turnaround. The built-in `w_dev` server recompiles your app as you make changes, and displays compiler errors in your browser (as well as the terminal, of course).
- High-performance. Wayward internally uses `libevent` and `libevhtp` with a threaded HTTP server by default, and it is already competitive with Go's `net/http` library in terms of requests per second. It essentially combines evented I/O with threaded processing of requests, and the system is designed from the ground up to be highly scalable.
- Batteries included. Wayward comes with an ORM system that integrates well with all components, but it doesn't force it down your throat. Wayward also comes with Wayward Support, a small class library with features commonly needed in web development that are missing from the STL.
- Wayward has a plugin for integrating with the templating framework [Synth](https://github.com/ajg/synth), that renders HTML or JSON templates using familiar Django syntax.

Getting Started
---------------

- `git clone https://github.com/simonask/w.git --recursive` — the recursive bit is important, because Wayward has a couple of dependencies.
- `cd w`
- `scons -j8`

And you're set. Build and run the Blog example:

- `cd examples/blog`
- `scons -j8`

Let's set up PostgreSQL for the blog:

- `createuser wayward_examples`
- `createdb wayward_examples_blog -O wayward_examples`
- `psql -d wayward_examples_blog < structure.sql`

And let's run it:

- `../../w_dev server blog`

You now have a server running at `localhost:3000` serving up a simple Blog application. You can just as easily run the `blog` executable directly, but then you won't get automatic recompilation when you muck around in the source code, and have to manually shut it down and run `scons`.

Example app
-----------

```C++
#include <w>

int main(int argc, char** argv) {
  w::App app { argv, argv };

  app.get("/", [](w::Request& req) -> w::Response {
    return w::render_text("Hello, World!");
  });

  app.post("/articles", [](w::Request& req) -> w::Response) {
    return w::render_text("You might have just created a new article!");
  });

  app.run();
}
```

Persistence Struct-Relational Mapper
------------------------------------

Persistence is an ORM included with Wayward that does several things:

- Builds SQL queries in a convenient and type-safe manner.
- Optionally defines relationships between objects in belongs-to/has-many/has-one relationships.
- Maps SQL result rows to instances of structs.

Persistence is non-intrusive — i.e., you can tell Persistence how to map a type to/from relational data without modifying the data structure.

Persistence is an approximate implementation of [the data mapper pattern](http://en.wikipedia.org/wiki/Data_mapper_pattern).

Example of binding a data structure to a table in an SQL database:

```C++
#include <p>

using p::PrimaryKey;
using p::BelongsTo;
using p::HasMany;
using w::DateTime;

struct Article {
  PrimaryKey      id;     // Could also be a plain integer, though less safe.
  DateTime        timestamp;
  std::string     title;
  std::string     text;
  BelongsTo<User> author; // Could also be an integer.
};

PERSISTENCE(Article) {
  relation("articles"); // Optional. Default = pluralized lowercase of the struct name.

  property(&Article::id, "id");
  property(&Article::timestamp, "timestamp");
  property(&Article::title, "title");
  property(&Article::text, "text");

  belongs_to(&Article::author, "author_id");
}
```

The second argument to `property(...)` above indicates the name of the column in the database.

Example of the query interface:

```C++
auto articles = from<Article>().where(p::column(&Article::title) == "The title.").order(&Article::timestamp).reverse_order();
auto articles_with_text = from<Article>().where(p::column(&Article::text).like("%something%"));

articles.each([&](Article& article) {
  // Do something with the fetched article.
});
```

The query interface is "lazy", and you can extend and modify queries in as many steps as you like. In the above, the only SQL command that actually gets executed happens when `each` is called.

Example of the update/insert interface (NIY):

```C++
Article article { ... };
p::insert(article); // always creates a new primary key
p::save(article);   // inserts if the object is new, otherwise updates.
```

Supported data stores:

- PostgreSQL

Why don't Wayward and Persistence use Boost?
--------------------------------------------

You can use Boost with Wayward, if you're so inclined. Wayward tries to avoid two things: Dependencies and complexity. Boost is a very complex dependency.

At the same time, the Wayward Support library includes a number of classes that could be said to replicate Boost functionality (such as `Maybe`, `CommandLineOptions`, `w::format`, and the serialization components). Why not simply use the Boost equivalents? It comes down to a difference in philosophy: Boost tries to be everything to everyone. Boost components rely heavily on very complex template metaprogramming techniques in order to achieve the maximum possible flexibility and performance under all circumstances. The result is code that is hard to understand and debug, as well as APIs that are sometimes awkward or difficult to use. It can also drastically increase compile times, which goes against the goal of a quick turnaround time and smooth development experience that Wayward represents.

Instead, Wayward often makes micro-sacrifices in performance, for instance by using runtime dynamic polymorphism instead of static template polymorphism, and as a consequence heap allocation in many places where Boost could have made do with faster stack allocation. The result is vastly more maintainable and readable code, which is important given the security concerns of an internet-facing web app, and the performance sacrifice is tolerable considering that most Wayward apps are strictly IO-bound, rather than CPU-bound.

Dependencies
------------

Dependency management in C++ apps is a pain, so `<w>` tries to keep it to a minimum.

- [libevent 2.0.x](http://libevent.org/)
- [Clang](http://clang.llvm.org/) and [libc++](http://libcxx.llvm.org/) — GCC will be supported in the future, but there are some disagreements between Clang++ and g++ that are holding back that development
- SCons 2.3.0 or newer.
- For Persistence: `libpq` (from PostgreSQL).

`<w>` has been tested on the following platforms:

- OS X 10.9.2 with Apple Clang-500.2.79


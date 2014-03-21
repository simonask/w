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

Example app
-----------

```C++
#include <w>

int main(int argc, char** argv) {
  w::App app;

  app.get("/", [](w::Request& req) -> w::Response {
    return w::render_text("Hello, World!");
  });

  app.post("/articles", [](w::Request& req) -> w::Response) {
    return w::render_text("You might have just created a new article!");
  });

  app.listen_and_serve("0.0.0.0", 3000);
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

struct Article {
  p::PrimaryKey   id;     // Could also be a plain integer, though less safe.
  uint64_t        timestamp;
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

Dependencies
------------

Dependency management in C++ apps is a pain, so `<w>` tries to keep it to a minimum.

- [libevent 2.0](http://libevent.org/)
- C++11 compatible compiler ([Clang](http://clang.llvm.org/) is recommended).
- For Persistence: `libpq` (from PostgreSQL).

`<w>` has been tested on the following platforms:

- OS X 10.9.2 with Apple Clang-500.2.79


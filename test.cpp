#include <w>
#include <p>
#include <iostream>

using p::PrimaryKey;
using p::BelongsTo;
using p::HasMany;

struct Article;

struct User {
  PrimaryKey id;
  std::string email;
  std::string crypted_password;
  HasMany<Article> articles;
};

PERSISTENCE(User) {
  property(&User::id, "id");
  property(&User::email, "email");
  property(&User::crypted_password, "crypted_password");
  has_many(&User::articles, "author_id");
}

struct Article {
  PrimaryKey id;
  std::string title;
  BelongsTo<User> author;
  std::string some_text;
};

PERSISTENCE(Article) {
  property(&Article::id, "id");
  property(&Article::title, "title");
  belongs_to(&Article::author, "author_id");
  property(&Article::some_text, "some_text");
}

int main (int argc, char const *argv[])
{
  w::App app;

  p::Configuration config = {
    .connection_string = "postgresql://wayward_test@localhost/wayward_test",
    .pool_size = 5,
  };

  std::string connection_error;
  if (p::connect(config, &connection_error)) {
    auto& p = p::get_connection();
    std::cout << w::format("Connected to PostgreSQL database: {0}@{1} on {2}\n", p.user(), p.database(), p.host());
  } else {
    std::cout << w::format("Connection failed: {0}\n", connection_error);
    return 1;
  }

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

  std::cout << w::format("Hello, World! article_id = {id}\n", {{"id", 123}});
  std::cout << w::format("Hello first argument: {0} {1} {0}\n", 1, 2);

  auto t = p::get_type<Article>();
  std::cout << w::format("Model type: {0}, relation = {1}\n", t->name(), t->relation());

  // auto articles = p::from<Article>().where("author_id = {0}", 1);
  // for (auto& article: articles) {

  // }

  using pr = p::relational_algebra;

  auto builder = pr::projection("articles")
    .where(
      (
        pr::column("articles", "title").like("%hej%")
        && pr::column("articles", "created_at") < "localtime()"
      ) || (
        pr::column("articles", "author_id") == 5
      )
    .order(pr::column("articles", "created_at"))
    .reverse_order()
  ;

  //return app.listen_and_serve("0.0.0.0", 3000);
  return 0;
}

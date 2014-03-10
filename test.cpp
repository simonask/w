#include <w>
#include <p>
#include <iostream>

struct Article;

struct User {
  int id;
  std::string email;
  std::string crypted_password;
  p::HasMany<Article> articles;
};

struct Article {
  int id;
  std::string title;
  p::BelongsTo<User> author;
};

PERSISTENCE(Article) {
  property(&Article::id, "id");
  property(&Article::title, "title");
  belongs_to(&Article::author, "author_id");
}

PERSISTENCE(User) {
  property(&User::id, "id");
  property(&User::email, "email");
  property(&User::crypted_password, "crypted_password");
  has_many(&User::articles, "author_id");
}

int main (int argc, char const *argv[])
{
  w::App app;

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

  //return app.listen_and_serve("0.0.0.0", 3000);
  return 0;
}

#include <w>
#include <p>
#include <iostream>

using p::PrimaryKey;
using p::BelongsTo;
using p::HasMany;
using w::Maybe;

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
  std::uint64_t created_at; // UNIX timestamp for now...
  std::string title;
  BelongsTo<User> author;
  Maybe<std::string> some_text;
};

PERSISTENCE(Article) {
  property(&Article::id, "id");
  property(&Article::title, "title");
  belongs_to(&Article::author, "author_id");
  property(&Article::some_text, "some_text");
  property(&Article::created_at, "created_at");
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
    std::stringstream ss;
    for (auto& pair: req.params) {
      ss << w::format("{0} = {1}\n", pair.first, pair.second);
    }

    auto article = p::from<Article>().order(&Article::id).reverse_order().first();
    if (article) {
      return w::render_text("{2}\nArticle {0}: {1}", article->id, article->title, ss.str());
    }
    return w::not_found();
    // return w::render_template("article", req);
    // return w::render_template("article", {{"id", 2}, {"title", "Hejsa"}});
  });

  app.get("/articles/redirect", [](w::Request& req) -> w::Response {
    return w::redirect("/");
  });

  app.post("/articles", [](w::Request& req) -> w::Response {
    return w::Response();
  });

  // auto articles = p::from<Article>().where("author_id = {0}", 1);
  // for (auto& article: articles) {

  // }

  namespace pr = p::relational_algebra;

  // auto query = pr::projection("articles")
  //   .where(
  //     (
  //       pr::column("articles", "title").like("%hej%")
  //       && pr::column("articles", "created_at") < pr::sql("localtime()")
  //     ) || (
  //       pr::column("articles", "author_id") == pr::literal(5)
  //     )
  //   )
  //   .left_join("users", "article_author",
  //     pr::column("article_author", "id") == pr::column("articles", "author_id")
  //   )
  //   .order(pr::column("articles", "created_at"))
  //   .reverse_order()
  // ;

  // std::string sql = p::get_connection().to_sql(*query.query);
  // std::cout << w::format("SQL:\n{0}\n", sql);

  // auto t = p::get_type<Article>();
  // std::cout << w::format("TYPE: {0}\nRELATION: {1}\n", t->name(), t->relation());
  // for (size_t i = 0; i < t->num_properties(); ++i) {
  //   auto& prop = t->property_at(i);
  //   std::cout << w::format("- {0} {1}\n", prop.type().name(), prop.column());
  // }
  // for (size_t i = 0; i < t->num_associations(); ++i) {
  //   auto& assoc = t->association_at(i);
  //   std::cout << w::format("@ [{0}] {1}\n", assoc.foreign_type().name(), assoc.foreign_key());
  // }

  // auto articles = p::from<Article>().where(
  //   (p::column(&Article::title).like("%hej%")
  //   && p::column(&Article::created_at).sql < p::sql("now()"))
  //   || (p::column(&Article::author) == 5)
  // );//.join(&Article::author).order(&Article::created_at).reverse_order();
  // std::string sql2 = articles.to_sql();
  // std::cout << w::format("SQL2:\n{0}\n", sql2);
  // articles.each([&](Article& a) {
  //   std::cout << w::format("Article {id}: {title}\n", {{"id", a.id}, {"title", a.title}});
  // });

  app.print_routes();

  return app.listen_and_serve("0.0.0.0", 3000);
  //return 0;
}

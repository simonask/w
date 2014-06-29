#include <gtest/gtest.h>

#include <persistence/projection.hpp>
#include <persistence/primary_key.hpp>
#include <persistence/persistence_macro.hpp>
#include <persistence/data_store.hpp>
#include <wayward/support/types.hpp>

#include "connection_mock.hpp"
#include "adapter_mock.hpp"

namespace {
  using persistence::PrimaryKey;
  using wayward::Maybe;
  using wayward::Nothing;
  using persistence::from;
  using persistence::column;
  using persistence::Context;

  using persistence::AdapterRegistrar;
  using persistence::test::ConnectionMock;
  using persistence::test::AdapterMock;

  struct Foo {
    PrimaryKey id;

    std::string string_value;
    Maybe<std::string> nullable_string_value;
    int32_t int32_value = -1;
    double double_value = -1;
  };

  PERSISTENCE(Foo) {
    property(&Foo::id, "id");
    property(&Foo::string_value, "string_value");
    property(&Foo::nullable_string_value, "nullable_string_value");
    property(&Foo::int32_value, "int32_value");
    property(&Foo::double_value, "double_value");
  }

  struct ProjectionTest : ::testing::Test {
    virtual ~ProjectionTest() {}
    AdapterRegistrar<AdapterMock> adapter_registrar_ = "test";
    Context context;

    persistence::test::ResultSetMock& results() {
      return *adapter_registrar_.adapter_.result_set_;
    }

    void SetUp() override {
      persistence::setup("test://test");
    }
  };

  struct ProjectionReturningSimpleColumns : ProjectionTest {
    void SetUp() override {
      ProjectionTest::SetUp();

      results().columns_ = {"foos_id", "foos_string_value", "foos_nullable_string_value", "foos_int32_value", "foos_double_value"};
      for (size_t i = 0; i < 5; ++i) {
        std::vector<Maybe<std::string>> row {
          wayward::format("{0}", i+1),
          wayward::format("String {0}", i),
          (i % 2 == 0 ? Maybe<std::string>(wayward::format("Nullable String {0}", i)) : Maybe<std::string>(Nothing)),
          wayward::format("{0}", (int32_t)(i*2)),
          wayward::format("{0}", ((double)i * 123.4))
        };
        results().rows_.push_back(std::move(row));
      }
    }
  };

  TEST_F(ProjectionReturningSimpleColumns, maps_primary_key) {
    auto q = from<Foo>(context);
    size_t counter = 0;
    q.each([&](const Foo& foo) {
      EXPECT_EQ(foo.id, counter+1);
      ++counter;
    });
    EXPECT_NE(0, counter);
  }

  TEST_F(ProjectionReturningSimpleColumns, maps_string_value) {
    auto q = from<Foo>(context);
    size_t counter = 0;
    q.each([&](const Foo& foo) {
      EXPECT_EQ(foo.string_value, *results().rows_.at(counter).at(1));
      ++counter;
    });
    EXPECT_NE(0, counter);
  }

  TEST_F(ProjectionReturningSimpleColumns, maps_nullable_string_value) {
    auto q = from<Foo>(context);
    size_t counter = 0;
    q.each([&](const Foo& foo) {
      EXPECT_EQ((bool)results().rows_.at(counter).at(2), (bool)foo.nullable_string_value);
      ++counter;
    });
    EXPECT_NE(0, counter);
  }

  TEST_F(ProjectionReturningSimpleColumns, maps_int32_value) {
    auto q = from<Foo>(context);
    size_t counter = 0;
    q.each([&](const Foo& foo) {
      std::stringstream ss;
      ss.str(*results().rows_.at(counter).at(3));
      int32_t n;
      ss >> n;
      EXPECT_EQ(n, foo.int32_value);
      ++counter;
    });
    EXPECT_NE(0, counter);
  }

  TEST_F(ProjectionReturningSimpleColumns, maps_double_value) {
    auto q = from<Foo>(context);
    size_t counter = 0;
    q.each([&](const Foo& foo) {
      std::stringstream ss;
      ss.str(*results().rows_.at(counter).at(4));
      double n;
      ss >> n;
      EXPECT_EQ(n, foo.double_value);
      ++counter;
    });
    EXPECT_NE(0, counter);
  }

  using persistence::BelongsTo;
  using persistence::HasMany;

  struct User;

  struct Article {
    PrimaryKey id;
    std::string title;
    std::string text;
    BelongsTo<User> author;
  };

  struct User {
    PrimaryKey id;
    std::string name;
    HasMany<Article> articles;
    BelongsTo<User> supervisor;
  };

  PERSISTENCE(Article) {
    property(&Article::id, "id");
    property(&Article::title, "title");
    property(&Article::text, "text");
    belongs_to(&Article::author, "author");
  }

  PERSISTENCE(User) {
    property(&User::id, "id");
    property(&User::name, "name");
    has_many(&User::articles, "articles", "author_id");
    belongs_to(&User::supervisor, "supervisor");
  }

  namespace w = wayward;

  struct ProjectionReturningArticlesWithUsers : ProjectionTest {
    void SetUp() override {
      ProjectionTest::SetUp();

      results().columns_ = {"articles_id", "articles_title", "articles_text", "articles_author_id", "users_id", "users_name", "users_supervisor_id"};
      for (size_t i = 0; i < 5; ++i) {
        std::vector<Maybe<std::string>> row {
          w::format("{0}", i+1), // Article::id
          w::format("Article {0}", i+1), // Article::title
          w::format("Text for article {0}.", i+1), // Article::text
          w::format("{0}", i+100), // Article::author_id
          w::format("{0}", i+100), // User::id
          w::format("User {0}", i+100), // User::name
          w::format("{0}", i+101) // User::supervisor_id
        };
        results().rows_.push_back(std::move(row));
      }
    }
  };

  TEST_F(ProjectionReturningArticlesWithUsers, joins_simple_belongs_to) {
    auto articles = from<Article>(context).inner_join(&Article::author);
    size_t counter = 0;
    articles.each([&](Article& article) {
      EXPECT_EQ(wayward::format("Article {0}", counter+1), article.title);
      EXPECT_NE(article.author, nullptr);
      EXPECT_EQ(wayward::format("User {0}", counter+100), article.author->name);
      ++counter;
    });
  }

  TEST_F(ProjectionReturningArticlesWithUsers, uses_simple_join_in_conditions) {
    auto articles = from<Article>(context).inner_join(&Article::author).where(column(&User::name).ilike("foo"));
  }

  TEST_F(ProjectionReturningArticlesWithUsers, renames_primary_table) {
    auto articles = from<Article>(context, "lol");
    auto sql = articles.to_sql();
    auto match = sql.find("FROM articles AS lol");
    EXPECT_NE(std::string::npos, match);
  }

  TEST_F(ProjectionReturningArticlesWithUsers, renames_joined_table) {
    auto articles = from<Article>(context).inner_join(&Article::author, "lol");
    auto sql = articles.to_sql();
    auto match = sql.find("INNER JOIN users AS lol ON");
    EXPECT_NE(std::string::npos, match);
  }

  TEST_F(ProjectionReturningArticlesWithUsers, joins_with_self) {
    auto users_with_supervisors = from<User>(context, "u").inner_join(&User::supervisor, "su");
  }

  TEST_F(ProjectionReturningArticlesWithUsers, refers_to_self_join_in_conditions) {
    auto users_with_supervisors = from<User>(context, "u").inner_join(&User::supervisor, "su").where(column("su", &User::name).ilike("foo"));
    auto sql = users_with_supervisors.to_sql();
    auto match = sql.find("WHERE \"su\".\"name\" ILIKE 'foo'");
    EXPECT_NE(std::string::npos, match);
  }

  TEST_F(ProjectionReturningArticlesWithUsers, selects_both_primary_and_joined_in_self_joins) {
    auto users_with_supervisors = from<User>(context, "u").inner_join(&User::supervisor, "su");
    auto sql = users_with_supervisors.to_sql();
    auto match1 = sql.find("\"u\".\"id\" AS \"u_id\"");
    auto match2 = sql.find("\"su\".\"id\" AS \"su_id\"");
    EXPECT_NE(std::string::npos, match1);
    EXPECT_NE(std::string::npos, match2);
  }

  TEST_F(ProjectionReturningArticlesWithUsers, performs_a_triple_self_join) {
    auto users_with_supervisors_and_their_supervisors = from<User>(context, "u0").inner_join(&User::supervisor, "u1").inner_join("u1", &User::supervisor, "u2");
    auto sql = users_with_supervisors_and_their_supervisors.to_sql();
    auto match = sql.find("INNER JOIN users AS u1 ON \"u0\".\"supervisor_id\" = \"u1\".\"id\" INNER JOIN users AS u2 ON \"u1\".\"supervisor_id\" = \"u2\".\"id\"");
    EXPECT_NE(std::string::npos, match);
  }
}

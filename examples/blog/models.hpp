#include <p>

using p::PrimaryKey;
using p::HasMany;
using p::BelongsTo;
using w::DateTime;

struct User;
struct Post;
struct Comment;

struct User {
  PrimaryKey id;
  std::string email;
  std::string crypted_password;
  std::string password_salt;
  std::string name;
  HasMany<Post> posts;
  HasMany<Comment> comments;
};

struct Post {
  PrimaryKey id;
  DateTime created_at;
  DateTime updated_at;
  DateTime published_at;
  std::string title;
  std::string text;

  BelongsTo<User> author;
  HasMany<Comment> comments;
};

struct Comment {
  PrimaryKey id;
  DateTime created_at;
  DateTime updated_at;
  std::string text;

  BelongsTo<User> author;
  BelongsTo<Post> post;
};

PERSISTENCE(User) {
  property(&User::id, "id");
  property(&User::email, "email");
  property(&User::crypted_password, "crypted_password");
  property(&User::password_salt, "password_salt");
  property(&User::name, "name");

  has_many(&User::posts, "posts", "author_id");
  has_many(&User::comments, "comments", "author_id");
}

PERSISTENCE(Post) {
  property(&Post::id, "id");
  property(&Post::created_at, "created_at");
  property(&Post::updated_at, "updated_at");
  property(&Post::published_at, "published_at");
  property(&Post::title, "title");
  property(&Post::text, "text");

  belongs_to(&Post::author, "author");
  has_many(&Post::comments, "comments", "post_id");
}

PERSISTENCE(Comment) {
  property(&Comment::id, "id");
  property(&Comment::created_at, "created_at");
  property(&Comment::updated_at, "updated_at");
  property(&Comment::text, "text");

  belongs_to(&Comment::author, "author");
  belongs_to(&Comment::post, "post");
}

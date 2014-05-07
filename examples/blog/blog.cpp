#include <w>

#include "models.hpp"

#include <fstream>

namespace app {
  using p::RecordPtr;

  template <typename T>
  w::Response json_response(const w::Request& req, T&& object, w::HTTPStatusCode code = w::HTTPStatusCode::OK) {
    w::Response response;
    // TODO: Set headers from req
    response.body = w::as_json(std::forward<T>(object));
    response.code = code;
    return response;
  }

  template <typename T>
  w::Response json_ok(const w::Request& req, T&& object) {
    return json_response(req, std::forward<T>(object));
  }

  struct PostRoutes : w::Routes {
    RecordPtr<Post> post;

    void before(w::Request& req) override {
      int64_t id;
      if (req.params["post_id"] >> id) {
        post = from<Post>().where(p::column(&Post::id) == id).first();
      }
    }

    w::Response get_all_posts(w::Request& req) {
      auto posts = from<Post>()
        .where(p::column(&Post::published_at) <= DateTime::now())
        .inner_join(&Post::author)
        .order(&Post::published_at)
        .reverse_order();
      auto all = posts.all();

      return json_ok(req, all);
    }

    w::Response get_post(w::Request& req) {
      return json_ok(req, post);
    }

    w::Response put_post(w::Request& req) {
      // TODO
      return w::not_found();
    }

    w::Response delete_post(w::Request& req) {
      return w::not_found();
    }

    w::Response get_comments(w::Request& req) {
      return w::not_found();
    }

    w::Response post_comment(w::Request& req) {
      return w::not_found();
    }
  };

  struct PostCommentRoutes : PostRoutes {
    RecordPtr<Comment> comment;

    void before(w::Request& req) override {
      PostRoutes::before(req);
      int64_t id;
      if (req.params["comment_id"] >> id) {
        comment = from<Comment>().where(p::column(&Comment::post) == post->id && p::column(&Comment::id) == id).first();
      }
    }

    w::Response get_comment(w::Request& req) {
      return json_ok(req, comment);
    }

    w::Response delete_comment(w::Request& req) {
      return w::not_found();
    }
  };

  w::Response crash(w::Request& req) {
    ::exit(1);
  }
}

int main(int argc, char const *argv[])
{
  p::setup("postgresql://wayward_examples@localhost/wayward_examples_blog");

  w::App app { argc, argv };

  app.get("/posts", &app::PostRoutes::get_all_posts);
  app.get("/posts/:post_id", &app::PostRoutes::get_post);
  app.patch("/posts/:post_id", &app::PostRoutes::put_post);
  app.del("/posts/:post_id", &app::PostRoutes::delete_post);
  app.get("/posts/:post_id/comments", &app::PostRoutes::get_comments);
  app.post("/posts/:post_id/comments", &app::PostRoutes::post_comment);
  app.get("/posts/:post_id/comments/:comment_id", &app::PostCommentRoutes::get_comment);
  app.del("/posts/:post_id/comments/:comment_id", &app::PostCommentRoutes::delete_comment);
  app.get("/crash", app::crash);

  app.get("/", [&](w::Request& req) -> w::Response {
    // TODO: A better way of reading templates/static files.
    std::ifstream f("index.html");
    if (!f.good()) {
      return w::not_found();
    }
    f.seekg(0, std::ios::end);
    size_t length = f.tellg();
    f.seekg(0, std::ios::beg);
    std::string resp;
    resp.resize(length);
    f.read(&resp[0], length);
    f.close();
    w::Response response;
    response.headers["Content-Type"] = "text/html";
    response.body = std::move(resp);
    return response;
  });

  return app.run();
}

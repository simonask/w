#include <w>

#include "models.hpp"

#include <fstream>

namespace app {
  using p::RecordPtr;
  using p::ValidationErrors;
  using w::redirect;
  using w::render;
  using w::format;
  using w::HTML;
  using w::JSON;

  struct PostsRoutes : w::Routes {
    w::Response get_all_posts(w::Request& req) {
      auto posts = from<Post>()
        .where(p::column(&Post::published_at) <= DateTime::now())
        .inner_join(&Post::author)
        .order(&Post::published_at)
        .reverse_order();

      return w::render("index.html", {{"posts", posts}});
    }
  };

  struct PostRoutes : w::Routes {
    RecordPtr<Post> post;

    void before(w::Request& req) override {
      int64_t id;
      if (req.params["post_id"] >> id) {
        post = from<Post>().where(p::eq(id, &Post::id)).inner_join(&Post::author).first();
      }
      if (!post)
        throw w::not_found();
    }

    w::Response get_post(w::Request& req) {
      return w::respond_to(req)
      .when<HTML>([&]() {
        auto comments = post->comments.scope().left_outer_join(&Comment::author).order(&Comment::created_at);
        return w::render("post.html", {{"post", post}, {"comments", comments}});
      })
      .when<JSON>([&]() {
        return w::render_json(post);
      });
    }

    w::Response put_post(w::Request& req) {
      p::assign_attributes(post, req.params["post"]);
      p::save(post);
      return w::respond_to(req)
      .when<HTML>([&]() { return w::redirect(w::format("/posts/{0}", post->id)); })
      .when<JSON>([&]() { return w::render_json(post); });
    }

    w::Response delete_post(w::Request& req) {
      // destroy(post);
      return w::redirect(w::format("/posts"));
    }

    w::Response get_comments(w::Request& req) {
      auto comments = post->comments.scope().left_outer_join(&Comment::author);
      return w::render("comments.html", {{"post", post}, {"comments", comments}});
    }

    w::Response post_comment(w::Request& req) {
      auto comment = create<Comment>(req.params["comment"]);
      comment->post = post;

      auto r = p::save(comment);
      if (r) {
        return redirect(format("/posts/{0}", post->id));
      } else {
        throw *r.error();
        //session.flash["error"] = r.error();
        return redirect(format("/posts/{0}/comments", post->id));
      }
    }
  };

  struct PostCommentRoutes : PostRoutes {
    RecordPtr<Comment> comment;

    void before(w::Request& req) override {
      PostRoutes::before(req);
      int64_t id;
      if (req.params["comment_id"] >> id) {
        comment = from<Comment>().where(p::eq(&Comment::post, post->id) && p::eq(id, &Comment::id)).first();
      }
    }

    w::Response get_comment(w::Request& req) {
      return w::render("comment.html", {{"comment", comment}});
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

  w::load_plugin("wayward_synth.plugin");
  w::set_template_engine("synth", {{"template_path", "views"}});

  w::App app { argc, argv };

  app.get("/", [](w::Request&) { return w::redirect("/posts"); });
  app.get("/posts", &app::PostsRoutes::get_all_posts);
  app.get("/posts/:post_id", &app::PostRoutes::get_post);
  app.patch("/posts/:post_id", &app::PostRoutes::put_post);
  app.del("/posts/:post_id", &app::PostRoutes::delete_post);
  app.get("/posts/:post_id/comments", &app::PostRoutes::get_comments);
  app.post("/posts/:post_id/comments", &app::PostRoutes::post_comment);
  app.get("/posts/:post_id/comments/:comment_id", &app::PostCommentRoutes::get_comment);
  app.del("/posts/:post_id/comments/:comment_id", &app::PostCommentRoutes::delete_comment);
  app.get("/crash", app::crash);

  app.get("/template", [&](w::Request& req) -> w::Response {
    return w::render("test.html", {{"message", "Hello, World!"}, {"params", req.params}});
  });

  return app.run();
}

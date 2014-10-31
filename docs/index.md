# Wayward Overview

[GitHub](https://github.com/simonask/w)

Did your dad ever tell you: "Writing web apps in C++ is dangerous and stupid! You should know better!"?

Well he's not the boss of you! You can do whatever you want!

Wayward ("`<w>`") is a web framework for writing HTTP-aware apps in C++. It uses [libevent](http://libevent.org/) behind
the scenes to create a compact HTTP server that can act as the backend for your app.

`<w>` is heavily inspired by the Ruby framework Sinatra, and aims to keep things as simple as possible.
It uses the C++ Standard Template Library extensively.

***WARNING!*** Wayward is as of yet still in early alpha, and should not, I repeat *NOT* be used in production just yet. Several big pieces are missing, and the rest is a semi-explosive mess that is probably unfit for human *and* animal consumption. Don't say you weren't warned. If you're looking for production-quality code, come back in a few releases, I'm sure we'll have something cooking for you.


## Getting Started

Wayward is both a library and a command-line utility, named `w_dev`. Wayward apps can exist without `w_dev`, but it does make life for developers a lot easier. `w_dev` creates projects, generates standardized components in projects, and has a built-in development server that automatically rebuilds and restarts your app when you make changes to it, allowing for live "Rails-style" coding.

### Building `w_dev`

1. Make sure dependencies are installed (git, scons, clang, libevent).
2. `git clone --recursive https://github.com/simonask/w.git`
3. `cd w`
4. `scons -j8 w_dev` (This will only build the necessary components for `w_dev`. Running with `-jN` makes scons run tasks in parallel.)

### Creating a Project

1. `cd ~/your/path/to/projects`
2. `path/to/w_dev new my_project`
3. Wait for `w_dev` to finish installing dependencies in your project dir.
4. You now have a project named `my_project` in `~/your/path/to/projects`.

### Building and Running

1. `cd my_project`
3. `path/to/w_dev server my_project`
4. Browse to `localhost:3000`. The first request in the lifetime of the project will take some time.

On the first request to the app server, `w_dev` will build your app. This may take about a minute the very first time because it also needs to build Wayward and dependencies, but subsequent requests will only cause a rebuild of the things that have changed in your own project. Normal turnaround for an incremental build should be very short.


## Sample "Hello World"

    #include <w>

    int main(int argc, char** argv) {
      w::App app { argc, argv };

      app.get("/", [](w::Request& req) {
        return w::render_text("Hello, World!");
      });

      return app.run();
    }

## Dependencies

Dependency management in C++ apps is a pain, so `<w>` tries to keep it to a minimum.

- [libevent 2.0.x](http://libevent.org/)
- [Clang](http://clang.llvm.org/) and [libc++](http://libcxx.llvm.org/) — GCC will be supported in the future, but there are some disagreements between Clang++ and g++ that are holding back that development
- SCons 2.3.0 or newer.
- For Persistence: `libpq` (from PostgreSQL).

`<w>` has been tested on the following platforms:

- OS X 10.9.2 with Apple Clang-503.0.40

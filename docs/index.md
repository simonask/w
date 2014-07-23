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

1. Make sure you have the dependencies installed.
2. `git clone --recursive https://github.com/simonask/w.git`
3. `cd w && scons -j4`
4. Include `<w>` in your cpp file, and start creating! Yay!

See the included examples in `w/examples`.

## Sample "Hello World"

    #include <w>

    int main(int argc, char** argv) {
      w::App app { argc, argv };

      app.get("/", [](w::Request& req) {
        return w::render_text("Hello, World!");
      });

      return app.run();
    }

Save as "hello.cpp" and compile with:

    clang++ -Iw -lwayward -lwayward_support -o hello hello.cpp

Run as:

    ./hello

## Dependencies

Dependency management in C++ apps is a pain, so `<w>` tries to keep it to a minimum.

- [libevent 2.0.x](http://libevent.org/)
- [Clang](http://clang.llvm.org/) and [libc++](http://libcxx.llvm.org/) — GCC will be supported in the future, but there are some disagreements between Clang++ and g++ that are holding back that development
- SCons 2.3.0 or newer.
- For Persistence: `libpq` (from PostgreSQL).

`<w>` has been tested on the following platforms:

- OS X 10.9.2 with Apple Clang-503.0.40

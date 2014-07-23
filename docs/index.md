# Wayward Overview

## Introduction

Wayward Web Framework is a simple web framework written in C++. It is designed to be easy to use, flexible without being complex, and safe.

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

    clang++ -Iw -o hello hello.cpp

Run as:

    ./hello

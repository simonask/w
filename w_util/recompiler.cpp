#include "recompiler.hpp"

#include <unistd.h>
#include <sys/param.h>
#include <stdlib.h>
#include <fcntl.h>

namespace wayward {
  namespace {
    void in_directory(const std::string& dir, std::function<void()> action) {
      // TODO: Make exception-safe.
      int old_cwd = ::open(".", O_RDONLY);
      int new_cwd = ::open(dir.c_str(), O_RDONLY);
      ::fchdir(new_cwd);
      action();
      ::fchdir(old_cwd);
    }
  }

  bool Recompiler::needs_rebuild() {
    int r;
    in_directory(directory_, [&]() {
      r = ::system("scons -q > /dev/null");
    });
    return r != 0;
  }

  void Recompiler::rebuild() {
    int r;
    in_directory(directory_, [&]() {
      r = ::system("scons");
    });
    if (r != 0) {
      // TODO: Display actual output.
      throw CompilationError(wayward::format("Build error."));
    }
  }
}

#include "recompiler.hpp"

#include <wayward/support/command_line_options.hpp>
#include <wayward/support/format.hpp>

#include <iostream>
#include <cstdlib>

static void usage(const std::string& program_name) {
  std::cout << wayward::format("Usage:\n\t{0} [app]\n\n", program_name);

  std::cout << "Options:\n";

  std::cout << "\t--port=<port>        Default: 3000\n";
  std::cout << "\t--address=<addr>     Default: 0.0.0.0\n";
  std::cout << "\t--no-live-recompile  Don't try to recompile the project on the fly\n";

  std::exit(1);
}

int main(int argc, char const *argv[])
{
  using namespace wayward;
  CommandLineOptions options {argc, argv};

  options.option("--help", "-h", [&]() {
    usage(argv[0]);
  });

  // TODO: Check that first argument is a path.

  return 0;
}

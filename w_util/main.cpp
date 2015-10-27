#include <iostream>
#include <string>
#include <cstdlib>

#include <wayward/support/format.hpp>

namespace w_dev {
  int server(int argc, char const* const* argv);
  int init(int argc, char const* const* argv);
  int generate(int argc, char const* const* argv);

  void usage(const char* program_name) {
    std::cerr << wayward::format("Usage:\n\t{0} [command]\n\n", program_name);
    std::cerr << "Options:\n";
    std::cerr << "\tserver [app]      Start a development server for binary 'app'. (Shorthand: s)\n";
    std::cerr << "\tnew [dir]         Create a new Wayward app in 'dir'.\n";
    std::cerr << "\tgenerate [thing]  Generate something in the current app dir. (Shorthand: g)\n";
    std::exit(1);
  }
}

int main(int argc, char const *argv[])
{
  if (argc < 2)
    w_dev::usage(argv[0]);

  std::string cmd = argv[1];

  if (cmd == "server" || cmd == "s") {
    return w_dev::server(argc - 1, argv + 1);
  } else
  if (cmd == "init" || cmd == "new") {
    return w_dev::init(argc - 1, argv + 1);
    w_dev::usage(argv[0]);
  } else
  if (cmd == "help" || cmd == "--help" || cmd == "-h") {
    w_dev::usage(argv[0]);
  } else
  if (cmd == "generate" || cmd == "g") {
    // TODO!
    // return w_dev::generate(argc - 1, argv + 1);
    w_dev::usage(argv[0]);
  } else {
    w_dev::usage(argv[0]);
  }

  return 0;
}

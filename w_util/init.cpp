#include <wayward/support/command_line_options.hpp>
#include <wayward/support/logger.hpp>
#include <wayward/support/format.hpp>
#include <wayward/support/string.hpp>

#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include <fstream>

namespace {
  using namespace wayward;

  void report_error(std::string err) {
    ConsoleStreamLogger::get()->log(Severity::Error, "new", err);
  }

  int run_command(std::string cmd, bool log = true) {
    if (log) {
      ConsoleStreamLogger::get()->log(Severity::Information, "new", format("=> {0}", cmd));
    }
    return ::system(cmd.c_str());
  }

  static bool path_exists(const std::string& path) {
    struct stat s;
    return ::stat(path.c_str(), &s) == 0;
  }

  static bool path_is_directory(const std::string& path) {
    struct stat st;
    int r = ::stat(path.c_str(), &st);
    return r == 0 && S_ISDIR(st.st_mode);
  }


  static const char app_template[] =
  "#include <w>\n"
  "\n"
  "int main(int argc, char** argv) {\n"
  "  w::App app { argc, argv };\n"
  "  \n"
  "  app.get(\"/\", [&](w::Request&) { return w::render_text(\"Hello, World!\"); });\n"
  "  \n"
  "  return app.run();\n"
  "}\n"
  ;

  static const char sconstruct_template[] =
  "import os\n"
  "\n"
  "WAYWARD_PATH = '{0}/' # Final slash important, otherwise rpath doesn't work.\n"
  "\n"
  "SConscript(os.path.join(WAYWARD_PATH, \"SConstruct\"))\n"
  "\n"
  "Import('env', 'wayward', 'persistence', 'wayward_support')\n"
  "\n"
  "env.Append(CPPPATH = WAYWARD_PATH)\n"
  "\n"
  "blog = env.Program('{1}', ['{2}'], LIBS = [wayward, persistence, wayward_support], LINKFLAGS = ['-rpath', WAYWARD_PATH])\n"
  ;
}

namespace w_dev {
  using namespace wayward;

  int init(int argc, char const* const* argv) {
    bool verbose = false;

    CommandLineOptions cmd;
    cmd.description("Print commands as they are executed.");
    cmd.option("--verbose", "-v", [&]() {
      verbose = true;
    });
    cmd.usage();
    auto names = cmd.parse(argc, argv);
    if (names.size() == 0) {
      cmd.display_usage_and_exit();
    }
    auto directory = names[0];
    auto path_components = split(directory, "/");
    auto appname = path_components.back();
    auto mainfile = format("{0}.cpp", appname);
    auto buildfile = "SConstruct";
    auto w_dir = ".w";

    ConsoleStreamLogger::get()->log(Severity::Information, "new", format("Creating app '{0}' in directory '{1}'...", appname, directory));

    if (run_command(format("mkdir -p {0}", directory), verbose) != 0) {
      report_error("mkdir failed, aborting.");
      return 1;
    }

    if (::chdir(directory.c_str()) != 0) {
      report_error("cd failed, aborting.");
      return 1;
    }

    ConsoleStreamLogger::get()->log(Severity::Information, "new", "Initializing Git repository...");

    if (run_command("git init .", verbose) != 0) {
      report_error("git init failed, aborting.");
      return 1;
    }

    if (path_exists(w_dir)) {
      ConsoleStreamLogger::get()->log(Severity::Information, "new", format("Submodule '{0}' already exists, skipping clone phase.", w_dir));
    } else {
      ConsoleStreamLogger::get()->log(Severity::Information, "new", "Adding Wayward submodules...");

      if (run_command(format("git submodule add https://github.com/simonask/w.git {0}", w_dir), verbose) != 0) {
        report_error("git submodule add failed, aborting.");
        return 1;
      }

      if (run_command("git submodule update --init --recursive", verbose) != 0) {
        report_error("git submodule update failed, aborting.");
        return 1;
      }
    }

    if (path_exists(mainfile)) {
      ConsoleStreamLogger::get()->log(Severity::Information, "new", format("Main file '{0}' already exists, skipping installation.", mainfile));
    } else {
      ConsoleStreamLogger::get()->log(Severity::Information, "new", format("Installing '{0}'...", mainfile));

      std::ofstream of { mainfile };
      of << std::string(app_template);
      of.close();
    }

    if (path_exists(buildfile)) {
      ConsoleStreamLogger::get()->log(Severity::Information, "new", format("Build file '{0}' already exists, skipping installation.", buildfile));
    } else {
      ConsoleStreamLogger::get()->log(Severity::Information, "new", format("Installing '{0}'...", buildfile));

      std::ofstream of { buildfile };
      of << format(sconstruct_template, w_dir, appname, mainfile);
      of.close();
    }

    return 0;
  }
}

#include <wayward/support/logger.hpp>
#include <wayward/support/format.hpp>
#include <wayward/support/datetime.hpp>

#include <fstream>
#include <iostream>

namespace wayward {
  void FormattedLogger::log(Severity severity, std::string tag, std::string message) {
    if (severity >= level_) {
      write_message(severity, wayward::format(format_, {
        {"start_color", ""},
        {"end_color", ""},
        {"severity", severity_as_string(severity)},
        {"timestamp", DateTime::now().strftime("%Y-%m-%d %H:%M:%S")},
        {"tag", std::move(tag)},
        {"message", std::move(message)}
      }));
    }
  }

  std::string severity_as_string(Severity severity) {
    switch (severity) {
      case Severity::Debug: return "DEBUG";
      case Severity::Information: return "INFO";
      case Severity::Warning: return "WARNING";
      case Severity::Error: return "ERROR";
    }
  }

  namespace {
    const char TerminalBlack[] =   "\033[22;30m";
    const char TerminalRed[] =     "\033[01;31m";
    const char TerminalGreen[] =   "\033[01;32m";
    const char TerminalMagenta[] = "\033[01;35m";
    const char TerminalCyan[] =    "\033[01;36m";
    const char TerminalYellow[] =  "\033[01;33m";
    const char TerminalResetColor[] = "\033[00m";

    static const char* CycleColors[] = {
      TerminalYellow,
      TerminalGreen,
      TerminalMagenta,
      TerminalCyan,
    };
    static size_t NumCycleColors = sizeof(CycleColors) / sizeof(const char*);
  }

  void ConsoleStreamLogger::log(Severity severity, std::string tag, std::string message) {
    if (severity >= level()) {
      const char* start_color = "";
      const char* end_color = "";

      if (colorize_) {
        uint32_t hash;
        if (severity == Severity::Error) {
          start_color = TerminalRed;
        } else {
          auto hash = std::hash<std::string>()(tag);
          auto color_idx = hash % NumCycleColors;
          start_color = CycleColors[color_idx];
        }
        end_color = TerminalResetColor;
      }

      write_message(severity, wayward::format(format(), {
        {"start_color", start_color},
        {"end_color", end_color},
        {"severity", severity_as_string(severity)},
        {"timestamp", DateTime::now().strftime("%Y-%m-%d %H:%M:%S")},
        {"tag", std::move(tag)},
        {"message", std::move(message)}
      }));
    }
  }

  struct FileLogger::Impl {
    std::fstream fs;
  };

  FileLogger::FileLogger(std::string path) : impl(new FileLogger::Impl) {
    impl->fs.open(path, std::ios_base::out);
    if (!impl->fs.good()) {
      std::cerr << wayward::format("ERROR: FileLogger could not open log file for output: {0}", path);
    }
  }

  FileLogger::~FileLogger() {}

  void FileLogger::write_message(Severity severity, std::string message) {
    impl->fs << message;
  }
}

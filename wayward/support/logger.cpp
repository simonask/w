#include <wayward/support/logger.hpp>
#include <wayward/support/format.hpp>
#include <wayward/support/datetime.hpp>

#include <fstream>
#include <iostream>

namespace wayward {
  namespace {
    std::string default_formatter(Severity severity, DateTime timestamp, const std::string& tag, const std::string& message) {
      if (severity == Severity::Warning || severity == Severity::Error) {
        return "{start_color}[{timestamp}] <{tag}> {severity}{end_color} {message}\n";
      } else {
        return "{start_color}[{timestamp}] <{tag}>{end_color} {message}\n";
      }

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

  FormattedLogger::FormattedLogger() : formatter_(default_formatter) {}

  void FormattedLogger::log(Severity severity, std::string tag, std::string message) {
    if (severity >= level_) {
      auto t = DateTime::now();
      auto format = formatter_(severity, t, tag, message);
      write_message(severity, wayward::format(format, {
        {"start_color", ""},
        {"end_color", ""},
        {"severity", severity_as_string(severity)},
        {"timestamp", t.strftime("%Y-%m-%d %H:%M:%S")},
        {"tag", std::move(tag)},
        {"message", std::move(message)}
      }));
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
      TerminalGreen,
      TerminalYellow,
      TerminalMagenta,
      TerminalCyan,
    };
    static size_t NumCycleColors = 4;
  }

  namespace {
    std::shared_ptr<ConsoleStreamLogger> g_console_logger;
  }

  std::shared_ptr<ILogger>
  ConsoleStreamLogger::get() {
    if (g_console_logger == nullptr) {
      g_console_logger = std::shared_ptr<ConsoleStreamLogger>(new ConsoleStreamLogger{std::cout, std::cerr});
    }
    return std::static_pointer_cast<ILogger>(g_console_logger);
  }

  void ConsoleStreamLogger::log(Severity severity, std::string tag, std::string message) {
    if (severity >= level()) {
      const char* start_color = "";
      const char* end_color = "";

      if (colorize_) {
        if (severity == Severity::Error) {
          start_color = TerminalRed;
        } else {
          // Locking because we might be modifying tag_colors_.
          std::unique_lock<std::mutex> L(mutex_);

          auto existing_color = tag_colors_.find(tag);
          size_t color_idx;
          if (existing_color == tag_colors_.end()) {
            color_idx = tag_colors_.size() % NumCycleColors;
            tag_colors_[tag] = color_idx;
          } else {
            color_idx = existing_color->second;
          }

          start_color = CycleColors[color_idx];
        }
        end_color = TerminalResetColor;
      }

      auto t = DateTime::now();
      auto format = formatter_(severity, t, tag, message);

      write_message(severity, wayward::format(format, {
        {"start_color", start_color},
        {"end_color", end_color},
        {"severity", severity_as_string(severity)},
        {"timestamp", t.strftime("%Y-%m-%d %H:%M:%S")},
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

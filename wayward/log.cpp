#include <wayward/w.hpp>
#include <wayward/support/logger.hpp>
#include <iostream>

namespace wayward {
  namespace {
    static std::shared_ptr<ILogger> g_current_logger;

    std::shared_ptr<ILogger> create_default_logger() {
      return ConsoleStreamLogger::get();
    }
  }

  std::shared_ptr<ILogger> logger() {
    if (g_current_logger == nullptr) {
      g_current_logger = create_default_logger();
    }
    return g_current_logger;
  }

  void set_logger(std::shared_ptr<ILogger> l) {
    g_current_logger = std::move(l);
  }

  namespace log {
    void debug(std::string tag, std::string message) {
      logger()->log(Severity::Debug, std::move(tag), std::move(message));
    }

    void debug(std::string message) {
      debug("app", message);
    }

    void info(std::string tag, std::string message) {
      logger()->log(Severity::Information, std::move(tag), std::move(message));
    }

    void info(std::string message) {
      info("app", message);
    }

    void warning(std::string tag, std::string message) {
      logger()->log(Severity::Warning, std::move(tag), std::move(message));
    }

    void warning(std::string message) {
      warning("app", message);
    }

    void error(std::string tag, std::string message) {
      logger()->log(Severity::Error, std::move(tag), std::move(message));
    }

    void error(std::string message) {
      error("app", std::move(message));
    }
  }
}

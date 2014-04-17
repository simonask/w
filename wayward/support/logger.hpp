#pragma once
#ifndef WAYWARD_SUPPORT_LOGGER_INCLUDED_HPP
#define WAYWARD_SUPPORT_LOGGER_INCLUDED_HPP

#include <string>
#include <memory>
#include <ostream>

namespace wayward {
  enum class Severity {
    Debug,
    Information,
    Warning,
    Error,
  };

  std::string severity_as_string(Severity severity);

  struct ILogger {
    virtual ~ILogger() {}
    virtual void log(Severity severity, std::string tag, std::string message) = 0;
    virtual Severity level() const = 0;
    virtual void set_level(Severity severity) = 0;
  };

  template <typename T, typename... Args>
  std::shared_ptr<ILogger> make_logger(Args&&... args) {
    return std::static_pointer_cast<ILogger>(std::make_shared<T>(std::forward<Args>(args)...));
  }

  struct FormattedLogger : ILogger {
    virtual ~FormattedLogger() {}

    // ILogger interface
    void log(Severity severity, std::string tag, std::string message) override;
    Severity level() const final { return level_; }
    void set_level(Severity l) final { level_ = l; }

    // FormattedLogger interface
    virtual void write_message(Severity severity, std::string formatted_message) = 0;
    std::string format() const { return format_; }
    void set_format(std::string fmt) { format_ = std::move(fmt); }
  private:
    Severity level_ = Severity::Debug;
    std::string format_ = "{start_color}{severity}: [{timestamp}] <{tag}>{end_color} {message}\n";
  };

  struct FileLogger : FormattedLogger {
    explicit FileLogger(std::string path);
    virtual ~FileLogger();

    void write_message(Severity severity, std::string formatted_message) final;

    struct Impl;
    std::unique_ptr<Impl> impl;
  };

  struct ConsoleStreamLogger : FormattedLogger {
    explicit ConsoleStreamLogger(std::ostream& out, std::ostream& err) : out_(out), err_(err) {}
    virtual ~ConsoleStreamLogger() {}

    bool colorize() const { return colorize_; }
    void set_colorize(bool b) { colorize_ = b; }

    void log(Severity severity, std::string tag, std::string message) override;

    void write_message(Severity severity, std::string formatted_message) final {
      if (severity >= Severity::Warning) {
        err_ << formatted_message;
      } else {
        out_ << formatted_message;
      }
    }

    std::ostream& out_;
    std::ostream& err_;
    bool colorize_ = true;
  };
}

#endif // WAYWARD_SUPPORT_LOGGER_INCLUDED_HPP

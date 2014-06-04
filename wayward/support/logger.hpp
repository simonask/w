#pragma once
#ifndef WAYWARD_SUPPORT_LOGGER_INCLUDED_HPP
#define WAYWARD_SUPPORT_LOGGER_INCLUDED_HPP

#include <string>
#include <memory>
#include <ostream>
#include <functional>
#include <mutex>

#include <wayward/support/datetime.hpp>

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

  using FormatFunction = std::function<std::string(Severity, DateTime timestamp, const std::string& tag, const std::string& message)>;

  struct FormattedLogger : ILogger {
    FormattedLogger();
    virtual ~FormattedLogger() {}

    // ILogger interface
    void log(Severity severity, std::string tag, std::string message) override;
    Severity level() const final { return level_; }
    void set_level(Severity l) final { level_ = l; }

    // FormattedLogger interface
    virtual void write_message(Severity severity, std::string formatted_message) = 0;
    void set_formatter(FormatFunction func) { formatter_ = std::move(func); }
  protected:
    Severity level_ = Severity::Debug;
    FormatFunction formatter_;
  };

  struct FileLogger : FormattedLogger {
    explicit FileLogger(std::string path);
    virtual ~FileLogger();

    void write_message(Severity severity, std::string formatted_message) final;

    struct Impl;
    std::unique_ptr<Impl> impl;
  };

  struct ConsoleStreamLogger : FormattedLogger {
    static std::shared_ptr<ILogger> get();

    bool colorize() const { return colorize_; }
    void set_colorize(bool b) { colorize_ = b; }

    void log(Severity severity, std::string tag, std::string message) override;

    void write_message(Severity severity, std::string formatted_message) final {
      std::unique_lock<std::mutex> L(mutex_);
      if (severity >= Severity::Warning) {
        err_ << formatted_message;
      } else {
        out_ << formatted_message;
      }
    }

  private:
    explicit ConsoleStreamLogger(std::ostream& out, std::ostream& err) : out_(out), err_(err) {}
    std::ostream& out_;
    std::ostream& err_;
    bool colorize_ = true;
    std::mutex mutex_;
    std::map<std::string, size_t> tag_colors_;
  };
}

#endif // WAYWARD_SUPPORT_LOGGER_INCLUDED_HPP

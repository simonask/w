#pragma once
#ifndef WAYWARD_SUPPORT_ERROR_HPP_INCLUDED
#define WAYWARD_SUPPORT_ERROR_HPP_INCLUDED

#include <stdexcept>
#include <wayward/support/format.hpp>

namespace wayward {
  std::vector<std::string> current_backtrace();
  std::string demangle_symbol(const std::string&);

  struct Error : std::runtime_error {
    explicit Error(const std::string& message) : std::runtime_error(message), backtrace_(current_backtrace()) {}
    virtual ~Error() {}

    const std::vector<std::string>& backtrace() const { return backtrace_; }
  private:
    std::vector<std::string> backtrace_;
  };

  template <typename ErrorType, typename... Args>
  void fail(const std::string& format, Args&&... formatting_arguments) {
    throw ErrorType{wayward::format(format, std::forward<Args>(formatting_arguments)...)};
  }
}

#endif // WAYWARD_SUPPORT_ERROR_HPP_INCLUDED

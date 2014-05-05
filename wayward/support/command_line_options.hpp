#pragma once
#ifndef WAYWARD_SUPPORT_COMMAND_LINE_OPTIONS_HPP_INCLUDED
#define WAYWARD_SUPPORT_COMMAND_LINE_OPTIONS_HPP_INCLUDED

#include <functional>
#include <string>
#include <vector>

#include <wayward/support/maybe.hpp>

namespace wayward {
  struct CommandLineOptions {
    CommandLineOptions();
    CommandLineOptions(int argc, char const* const* argv);
    void set(int argc, char const* const* argv);

    void option(const std::string& long_form, const std::string& short_form, std::function<void()> callback);
    void option(const std::string& long_form, const std::string& short_form, std::function<void(const std::string&)> callback_with_argument);
    void option(const std::string& long_form, const std::string& short_form, std::function<void(int64_t)> callback_with_argument);
    void option(const std::string& long_form, std::function<void()> callback);
    void option(const std::string& long_form, std::function<void(const std::string&)> callback_with_argument);
    void option(const std::string& long_form, std::function<void(int64_t)> callback_with_argument);

  private:
    bool find_option(const std::string& o);
    Maybe<std::string> find_option_with_value(const std::string& o, bool allow_equals = false);
    std::vector<std::string> options_;
  };
}

#endif // WAYWARD_SUPPORT_COMMAND_LINE_OPTIONS_HPP_INCLUDED

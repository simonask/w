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
    void program_name(std::string); // Defaults to argv[0]

    /*
      parse returns a list of unmatched non-option arguments, and everything after "--".
    */
    std::vector<std::string>
    parse(int argc, char const* const* argv);

    void description(std::string next_option_description);

    void option(std::string long_form, std::string short_form, std::function<void()> callback);
    void option(std::string long_form, std::string short_form, std::function<void(std::string)> callback_with_argument);
    void option(std::string long_form, std::string short_form, std::function<void(int64_t)> callback_with_argument);
    void option(std::string long_form, std::function<void()> callback);
    void option(std::string long_form, std::function<void(std::string)> callback_with_argument);
    void option(std::string long_form, std::function<void(int64_t)> callback_with_argument);

    void usage(std::string long_form = "--help", std::string short_form = "-h");
    void usage(std::function<void()> action, std::string long_form = "--help", std::string short_form = "-h");
    void unrecognized(std::function<void(std::string)>);

    void display_usage();
    void display_usage_and_exit();

  private:
    Maybe<std::string> program_name_;
    struct Option {
        Maybe<std::string> long_form;
        Maybe<std::string> short_form;
        std::string description;
        std::function<void(Maybe<std::string>)> action;
        bool needs_value;
    };
    std::vector<Option> options_;
    std::string next_option_description_;
    size_t longest_long_form_ = 0;
    size_t longest_short_form_ = 0;
    std::function<void(std::string)> unrecognized_action_;

    void add_option(Maybe<std::string> long_form, Maybe<std::string> short_form, std::function<void(Maybe<std::string>)> action, bool needs_value = false);
  };

  inline void CommandLineOptions::program_name(std::string pn) {
    program_name_ = std::move(pn);
  }

  inline void CommandLineOptions::description(std::string next_option_description) {
    next_option_description_ = std::move(next_option_description);
  }

  inline void CommandLineOptions::unrecognized(std::function<void(std::string)> action) {
    unrecognized_action_ = std::move(action);
  }
}

#endif // WAYWARD_SUPPORT_COMMAND_LINE_OPTIONS_HPP_INCLUDED

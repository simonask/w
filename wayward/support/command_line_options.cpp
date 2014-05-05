#include <wayward/support/command_line_options.hpp>

namespace wayward {
  CommandLineOptions::CommandLineOptions() {}

  CommandLineOptions::CommandLineOptions(int argc, char const* const* argv) {
    set(argc, argv);
  }

  void CommandLineOptions::set(int argc, char const* const* argv) {
    options_.clear();
    for (int i = 1; i < argc; ++i) {
      options_.push_back(argv[i]);
    }
  }

  bool CommandLineOptions::find_option(const std::string& o) {
    for (auto& it: options_) {
      if (it == o) {
        return true;
      }
    }
    return false;
  }

  Maybe<std::string> CommandLineOptions::find_option_with_value(const std::string& o, bool allow_eq) {
    std::string option_with_eq = allow_eq ? wayward::format("{0}=", o) : std::string();

    for (size_t i = 0; i < options_.size(); ++i) {
      if (o == options_.at(i)) {
        if (i+1 != options_.size()) {
          return options_.at(i+1);
        } else {
          return std::string();
        }
      } else if (allow_eq) {
        if (o.find(option_with_eq) == 0) {
          auto pos_eq = o.find("=");
          return o.substr(pos_eq+1);
        }
      }
    }

    return Nothing;
  }

  void CommandLineOptions::option(const std::string& long_form, const std::string& short_form, std::function<void()> callback) {
    auto option = find_option(long_form);
    if (!option) {
      option = find_option(short_form);
    }
    if (option) {
      callback();
    }
  }

  void CommandLineOptions::option(const std::string& long_form, const std::string& short_form, std::function<void(const std::string&)> callback) {
    auto option = find_option_with_value(long_form, true);
    if (!option) {
      option = find_option_with_value(short_form);
    }
    if (option) {
      callback(*option);
    }
  }

  void CommandLineOptions::option(const std::string& long_form, const std::string& short_form, std::function<void(int64_t)> callback) {
    auto option = find_option_with_value(long_form, true);
    if (!option) {
      option = find_option_with_value(short_form);
    }
    if (option) {
      std::stringstream ss;
      ss << *option;
      int64_t i;
      ss >> i;
      callback(i);
    }
  }

  void CommandLineOptions::option(const std::string& long_form, std::function<void()> callback) {
    auto option = find_option(long_form);
    if (option) {
      callback();
    }
  }

  void CommandLineOptions::option(const std::string& long_form, std::function<void(const std::string&)> callback) {
    auto option = find_option_with_value(long_form, true);
    if (option) {
      callback(*option);
    }
  }

  void CommandLineOptions::option(const std::string& long_form, std::function<void(int64_t)> callback) {
    auto option = find_option_with_value(long_form, true);
    if (option) {
      std::stringstream ss;
      ss << *option;
      int64_t i;
      ss >> i;
      callback(i);
    }
  }

}

#include <wayward/support/command_line_options.hpp>

#include <iostream>
#include <cstdlib>

namespace wayward {
  CommandLineOptions::CommandLineOptions() {}

  void CommandLineOptions::add_option(Maybe<std::string> long_form, Maybe<std::string> short_form, std::function<void(Maybe<std::string>)> action, bool needs_value) {
    if (long_form) {
      longest_long_form_ = std::max(long_form->size(), longest_long_form_);
    }

    if (short_form) {
      longest_short_form_ = std::max(short_form->size(), longest_short_form_);
    }

    options_.emplace_back((Option){
      std::move(long_form),
      std::move(short_form),
      std::move(next_option_description_),
      std::move(action),
      needs_value
    });
  }

  std::vector<std::string>
  CommandLineOptions::parse(int argc, char const* const* argv) {
    int start_at = program_name_ ? 0 : 1;
    if (argc <= start_at)
      return std::vector<std::string>();

    if (!program_name_) {
      program_name_ = std::string(argv[0]);
    }

    std::map<std::string, std::vector<Option>::iterator> options_index;
    for (auto it = options_.begin(); it != options_.end(); ++it) {
      if (it->long_form) {
        options_index[*it->long_form] = it;
      }
      if (it->short_form) {
        options_index[*it->short_form] = it;
      }
    }

    bool after_double_dash = false;
    std::vector<std::string> words;

    for (int i = start_at; i < argc; ++i) {
      std::string o = argv[i];

      if (after_double_dash || o[0] != '-') {
        words.push_back(std::move(o));
        continue;
      }

      if (o == "--") {
        after_double_dash = true;
        continue;
      }

      auto eq_pos = o.find('=');
      Maybe<std::string> value;
      std::vector<Option>::iterator it = options_.end();
      if (eq_pos != std::string::npos) {
        value = o.substr(eq_pos + 1);
        o = o.substr(0, eq_pos);
        auto idx_it = options_index.find(o);
        if (idx_it != options_index.end()) {
          it = idx_it->second;
        }
      } else {
        auto idx_it = options_index.find(o);
        if (idx_it != options_index.end()) {
          it = idx_it->second;
          if (it->needs_value) {
            if (i+1 < argc) {
              value = std::string(argv[i+1]);
              ++i;
            } else {
              std::cerr << wayward::format("Option '{0}' expects a value.\n");
              display_usage_and_exit();
            }
          }
        }
      }

      if (it == options_.end()) {
        if (unrecognized_action_) {
          unrecognized_action_(std::move(o));
        }
      } else {
        it->action(std::move(value));
      }
    }

    return words;
  }

  void CommandLineOptions::usage(std::string long_form, std::string short_form) {
    CommandLineOptions* self = this;
    usage([=]() {
      self->display_usage_and_exit();
    }, std::move(long_form), std::move(short_form));
  }

  void CommandLineOptions::usage(std::function<void()> action, std::string long_form, std::string short_form) {
    add_option(std::move(long_form), std::move(short_form), [=](Maybe<std::string>) { action(); });
  }

  void CommandLineOptions::display_usage() {
    std::cout << "/* TODO: CommandLineOptions::display_usage */\n";
  }

  void CommandLineOptions::display_usage_and_exit() {
    display_usage();
    std::exit(1);
  }

  void CommandLineOptions::option(std::string long_form, std::string short_form, std::function<void()> callback) {
    add_option(std::move(long_form), std::move(short_form), [=](Maybe<std::string>) { callback(); }, false);
  }

  void CommandLineOptions::option(std::string long_form, std::string short_form, std::function<void(std::string)> callback) {
    add_option(std::move(long_form), std::move(short_form), [=](Maybe<std::string> value) { callback(*value); }, true);
  }

  void CommandLineOptions::option(std::string long_form, std::string short_form, std::function<void(int64_t)> callback) {
    add_option(std::move(long_form), std::move(short_form), [=](Maybe<std::string> value) {
      std::stringstream ss(*value);
      int64_t n;
      ss >> n;
      callback(n);
    }, true);
  }

  void CommandLineOptions::option(std::string long_form, std::function<void()> callback) {
    add_option(std::move(long_form), Nothing, [=](Maybe<std::string>) { callback(); }, false);
  }

  void CommandLineOptions::option(std::string long_form, std::function<void(std::string)> callback) {
    add_option(std::move(long_form), Nothing, [=](Maybe<std::string> value) { callback(*value); }, true);
  }

  void CommandLineOptions::option(std::string long_form, std::function<void(int64_t)> callback) {
    add_option(std::move(long_form), Nothing, [=](Maybe<std::string> value) {
      std::stringstream ss(*value);
      int64_t n;
      ss >> n;
      callback(n);
    }, true);
  }

}

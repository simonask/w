# Class: CommandLineOptions

[wayward/support/command_line_options.hpp](https://github.com/simonask/w/blob/master/wayward/support/command_line_options.hpp)

`CommandLineOptions` is a very simple command line option parser.

Example usage:

    CommandLineOptions cmd;

    cmd.description("Provide option 'foo'.");
    cmd.option("--foo", "-f", []() { ... });

    cmd.description("Set the number with --number=123 or -n 123.");
    cmd.option("--number", "-n", [](int64_t) { ... });

    cmd.description("Provide a string with --string=foo or -s foo");
    cmd.option("--string", "-s", [](std::string) { ... });

    cmd.usage();
    cmd.parse(argc, argv);

`CommandLineOptions` recognizes the `--help` and `-h` options by default after the call to [`usage()`](#usage), and displays all available options with the associated description (if any). The closures provided with the [`option`](#option) are run when the option is encountered during [`parse`](#parse).

---

# Constructors

## CommandLineOptions()

Empty `CommandLineOptions`.

---

# Methods

---

## program_name

Invoke: `program_name(std::string)`

Set the program name. Defaults to `argv[0]`. If a program name is provided manually, `argv[0]` is interpreted as an option instead of the binary path.

## description

Invoke: `description(std::string)`

Set the description of the next option that is defined. Each successive call to `description` overwrites the previous value.

## option

Invoke: `option(long_form, short_form, callback)` or `option(long_form, callback)`

`callback` is either an `std::function<void()>`, or an `std::function<void(std::string)>`, or an `std::function<void(int64_t)>`. If the callback takes no parameters, the option is assumed to not take a value argument. If the callback takes a string argument, the value argument is passed directly. If the callback takes an integer argument, the value argument is converted to an integer first.

## usage

Invoke: `usage(long_form, short_form)` or `usage(action, long_form, short_form)`

Configures the "usage" display. By default, `CommandLineOptions` interprets the options `--help` and `-h` and calls [`display_usage_and_exit()`](#displayusageandexit).

## unrecognized

Invoke: `unrecognized(callback)`

`callback` is an `std::function<void(std::string)>`, and it will be invoked for each unrecognized option. By default, `CommandLineOptions` does nothing when encountering an unrecognized option.

## display_usage

Invoke: `display_usage()`

Print usage text to `std::cout`.

## display_usage_and_exit

Invoke: `dispay_usage_and_exit()`

Print usage text to `std::cout` and call `std::exit(1)`.

## parse

Invoke: `parse(argc, argv)`

Returns: `std::vector<std::string>` (a list of unrecognized command line options)

Parse `argc` arguments of `argv` and execute the relevant actions.



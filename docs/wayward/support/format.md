# Library: format

[wayward/support/format.hpp](https://github.com/simonask/w/blob/master/wayward/support/format.hpp)

`w::format` is the main entry point for string interpolation in Wayward. It has two main modes of operation: Numbered and Named interpolation.

`w::format` uses the standard `operator<<(std::ostream&)` interface internally to convert arguments to strings. This may or may not change in the future.

# Numbered Interpolation

This performs positional interpolation, and expects string placeholders in the format *{n}*, where *n* is a number starting at 0.

Example:

    w::format("Hello, {0}!", "World"); // => "Hello, World!"

Example with numbers:

    w::format("{0} + {1} = {2}", 123, 456, 579); // => "123 + 456 = 579"

# Named Interpolation

Instead of positional arguments, `w::format` also supports named arguments, like so:

    w::format("Hello, {entity}! I am {subject}.", {
      {"entity", "World"},
      {"subject", "Simon"}
    }); // => "Hello, World! I am Simon."

# Output formatting

TODO (NIY)

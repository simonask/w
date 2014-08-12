# Wayward Support API

Wayward Support is a collection of classes and functions designed to work in addition to the C++ Standard Library.

Some classes in Wayward Support are simpler implementations of equivalents in Boost without as much template cruft, while implement concepts that are unique to Wayward.

## Classes

- [Any](any.md) — Hold any value.
- [CommandLineOptions](command_line_options.md) — Simple command line option parser.
- [Either](either.md) — Hold one of a predefined set of value types (tagged union).
- [Error](error.md) — Base class for all exceptions that captures a stack trace.
- [Fiber](fiber.md) — Coroutine implementation for cooperative multitasking.
- [Logger](logger.md) — Logging interface.
- [Maybe](maybe.md) — Option type, equivalent to `boost::optional` with some monadic extensions.
- [Result](result.md) — Either a value or an Error.
- [URI](uri.md) — Structured representation of a Uniform Resource Identifier.

## Libraries

- [Data Franca library](data_franca.md) — Data observation library. Provides unified access to any data type that implements an adapter, allowing users to query arbitrary data structures. The main use is to provide a way for templating languages to access structured data at runtime.
- [DateTime library](datetime.md) — The DateTime library provides an intuitive interface to POSIX date/time functions.
- [Format library](format.md) — String formatting and interpolation library.
- [HTTP library](http.md) — Evented HTTP Server and Client classes.
- [JSON library](json.md) — Convert arbitrary data structures (via [reflection](reflection.md) or [Data Franca](data_franca.md)) to and from JSON.
- [Monads](monads.md) — Common "monadic" idioms that can make some code patterns safer and easier to use.
- [Reflection library](reflection.md) — Runtime type reflection library.
- [Static Metaprogramming library](static.md) — Compile-time type reflection library (various extensions to the standard `<type_traits>`).
- [String utilities](string.md) — Commonly used string functions missing from the C++ Standard Library.

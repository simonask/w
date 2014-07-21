# Class: Error

[wayward/support/error.hpp](https://github.com/simonask/w/blob/master/wayward/support/error.hpp)

`Error` is a base class for all exceptions that want to capture a stack trace.

It derives from `std::runtime_error` so as to be compatible with standard C++ error handling.

---

# Constructors

---

## Error(const std::string& message)

Construct an `Error` with a `message`.

---

# Methods

---

## what

Invoke: `what()`

Returns: `const char*` pointing to an error message describing the message.

## backtrace

Invoke: `backtrace()`

Returns: `std::vector<std::string>` — a list of stack trace entries (demangled where applicable), in descending order from call site to program entry.

# Library: String utilities

[wayward/support/string.hpp](https://github.com/simonask/w/blob/master/wayward/support/string.hpp)

---

# Functions

---

## split

Use: `split(input, delimiter)` or `split(input, delimiter, max)`

Returns: `std::vector<std::string>`

Splits the input string by delimiter and returns a vector of string, not including the delimiters. If `max` is provided, a maximum of `max` strings are returned.

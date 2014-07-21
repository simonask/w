# Class: URI

[wayward/support/uri.hpp](https://github.com/simonask/w/blob/master/wayward/support/uri.hpp)

`URI` represents a [Uniform Resource Identifier](http://en.wikipedia.org/wiki/Uniform_resource_identifier).

---

# Constructors

---

## URI()

An empty URI.

## URI(scheme, host, port, path, query, fragment)

An URI representing `scheme://host:port/path?query#fragment`. Each component, except for host and scheme, may be empty.

## URI::parse

Invoke: `URI::parse(input)`

Returns: `Maybe<URI>`

Parses the `input` string and returns a `Maybe<URI>`. If the input string failed to parse as an URI, the return value is `Nothing`.

---

# Members

---

## scheme

std::string

## username

std::string

## password

std::string

## host

std::string

## port

int

## path

std::string

## query

std::string

## fragment

std::string


---

# Methods

---

## to_string

Use: `to_string()`

Returns: `std::string` — String representation of the URI.

---

# Static Methods

---

## URI::decode

Use: `URI::decode(input)`

Returns: `std::string` with UTF-8 encoded characters.

Decodes URI entities.

## URI::encode

Use: `URI::encode(utf8)`

Returns: `std::string` with URI-encoded characters.

Encodes URI entities.

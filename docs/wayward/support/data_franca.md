# Data Franca API

Github: [wayward/support/data_franca](https://github.com/simonask/w/tree/master/wayward/support/data_franca)

Namespace: `wayward::data_franca`

Data Franca is a data observation/mutation library. It provides a single, unified interface to querying and modifying arbitrary data structures.

Possible use cases:

- Serialization
- Data binding
- Interfacing with scripting languages, namely template languages

Data Franca is based around 4 core concepts:

- **[Spectators](#spectators):** Query objects providing a read-only interface to any data structure.
- **[Mutators](#mutators):** Same as spectators, except they can also modify data structures.
- **[Adapters](#adapters):** Interface classes that bind the spectator/mutator interface to arbitrary data types. Simply put, to be able to inspect or mutate a custom object, an adapter needs to be defined for it.
- **[Objects](#objects):** Opaque, mutable object structure that always supports all operations. This is handy for building structured data on the fly, e.g. constructing a JSON document. Objects behave like mutators, except they own their data.

---

# Spectators

---

Making a spectator can be constructed from a reference to any object that has an [adapter](#adapters).

    std::map<std::string, std::string> dict = {{"foo", "bar"}, {"baz", "car"}};

    Spectator spec { dict };
    std::string result;
    if (spec["foo"] >> result) {
      std::cout << result; // Will print "bar".
    }

Scalar values (strings, numbers) are extracted with `operator>>`, which returns a boolean that is true if the value could be successfully extracted to the requested type. Extraction attempts to convert the underlying value to the requested type, so if the underlying type is a string, but an integer or float is requested, it will be interpreted as a string representation of a number. Extraction as string always succeeds, unless the accessed value is a dictionary or list.

Compound values (dictionaries, lists) are accessed with `operator[]`. If called with a string argument, the value is assumed to be a dictionary, and if called with an integer argument, it is assumed to be a list. If the underlying type is not the requested container type (e.g. if a list or scalar is accessed with a string subscript), an empty spectator representing `Nothing` will be returned.

## type

Returns: `DataType`

One of `DataType::Nothing`, `DataType::Boolean`, `DataType::Integer`, `DataType::Real`, `DataType::String`, `DataType::List`, `DataType::Dictionary`.

## is_nothing

Returns: `bool`

True if the represented value is conceptually "nothing" (`Nothing`, `NULL`, etc.).

## operator bool

Returns: `bool`

True if the represented value is not conceptually "nothing". Reverse of [is_nothing](#isnothing).

## operator>>(Boolean&)

Returns: `bool`

Extract boolean value. Returns true if the underlying type is a boolean, or could be converted to a boolean.

## operator>>(Integer&)

Returns: `bool`

Extract integer value. Returns true if the underlying type is an integer, or could be converted to an integer.

## operator>>(Real&)

Returns: `bool`

Extract float value. Returns true if the underlying type is a float, or could be converted to a float.

## operator>>(String&)

Returns: `bool`

Extract string value. Returns true if the underlying type is a string, or could be converted to a string.

## operator[](std::string)

Returns: `Spectator`

Access value as a dictionary. If the value is not a dictionary, or the key does not exist in the dictionary, a spectator representing "nothing" is returned.

## operator[](size_t)

Returns: `Spectator`

Access value as a list. If the value is not a list, or the index is out of bounds, a spectator representing "nothing" is returned.

## has_key

Invoke: `has_key(key)`

Returns: `bool`

True if the underlying value is a dictionary and has key `key`.

## length

Invoke: `length()`

Returns: `size_t`

If the underlying value is a dictionary or list, returns the number of pairs or elements. Otherwise, returns 0.

## begin

Invoke: `begin()`

Returns: An iterator that can be used to enumerate all elements or pairs of the underlying list or dictionary, or "end" if they are empty, or "end" if the underlying value is not a container.

## end

Invoke: `end()`

Returns: An iterator pointing to the conceptual "end" (nothing). May never be dereferenced.

---

# Mutators

---

Mutators are used like [Spectators](#spectators), except they can also modify the underlying data structures.

Example:

    std::map<std::string, std::string> dict = {{"foo", "bar"}, {"baz", "car"}};

    Mutator mut { dict };
    if (mut["foo"] << "lol") {
      // dict["foo"] == "lol"
    }

In contrast to spectators, type coercion is enforced at the adapter level — some adapters allow changing the type of represented values at runtime, while other don't, so the mutator interface can't make the decision. Therefore, setting a value can fail, and the return value of `operator<<` determines if the write was successful.

In addition to the interface provided by [Spectators](#spectators), they implement the following:

## operator<<(NothingType)

Returns: `bool`

Set the represented value to `Nothing`.

## operator<<(Boolean)

Returns: `bool`

Set the represented value to the boolean value.

## operator<<(Integer)

Returns: `bool`

Set the represented value to the integer value.

## operator<<(Real)

Returns: `bool`

Set the represented value to the floating-point value.

## operator<<(String)

Returns: `bool`

Set the represented value to the string value.

## operator[](std::string)

Returns: `Mutator`

Access dictionary value. Some adapters may coerce the underlying type to become a dictionary.

## operator[](size_t)

Returns: `Mutator`

Access list element. Some adapters may coerce the underlying type to become a list.

## erase

Invoke: `erase(std::string)`

Returns: `bool`

Erase the dictionary element. Returns true if the erase was successful.

## push_back

Invoke: `push_back(value)`

Returns: `bool`

Append a value to the end of a list. Some adapters may coerce the underlying type to become a list.


---

# Adapters

---

## Implementing custom adapters

To allow inspection of a custom type, implement `Adapter<T>`, where `T` is your custom type.

`Adapter<T>` must implement the interface `IAdapter`, which implies `IReader` and `IWriter`.

Relevant interfaces:

- IAdapter: [wayward/support/data_franca/adapter.hpp](https://github.com/simonask/w/blob/master/wayward/support/data_franca/adapter.hpp)
- IReader: [wayward/support/data_franca/reader.hpp](https://github.com/simonask/w/blob/master/wayward/support/data_franca/reader.hpp)
- IWriter: [wayward/support/data_franca/writer.hpp](https://github.com/simonask/w/blob/master/wayward/support/data_franca/writer.hpp)

---

# Objects

---

A Data Franca Object implements the [Mutator](#mutators) interface (including the [Spectator](#spectators) interface), but owns its own data.

Write-operations on objects always convert the internal type of the object to match the corresponding write operation. Therefore, a write operation on an Object never fails.

Example:

    Object o;
    o["foo"]["bar"] << 123;
    o["lol"].push_back("baz");
    o["lol"].push_back("boo");

If `o` in the above were converted to, say, JSON, it would look like this:

    {
      "foo": {
        "bar": 123
      },
      "lol": ["baz", "boo"]
    }

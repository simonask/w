# Class: Any

[wayward/support/any.hpp](https://github.com/simonask/w/blob/master/wayward/support/any.hpp)

`Any` can contain any object by value. When an `Any` is constructed, the input value is copied or moved into the `Any` instance's private storage as well as a piece of unique type information.

The underlying type of an `Any` instance can be queried with the method [`is_a<T>()`](#is_a), which returns a boolean. The held value can be accessed with [`get<T>()`](#get), which returns a [`Maybe<T>`](maybe.md). If the held value is not of type `T`, `Nothing` is returned.

The held value can also be accessed by reference by passing a closure to [`when<T>(...)`](#whenclosure), where the closure will only be run if the type matches. The argument to the closure will be a reference to the held value.

The two related classes `AnyRef` and `AnyConstRef` behave identical to `Any`, except they don't own the value they refer to. Instead, they hold a reference and a const reference, respectively. Care must be taken to ensure that the lifetime of the `AnyRef` or `AnyConstRef` does not exceed the lifetime of the object they refer to (as a rule of thumb: use `AnyRef`/`AnyConstRef` in method signatures only, and never store the reference).

---

# Constructors

---

## Any(T&& value)

Construct an `Any` by moving `value`.

## Any(const T& value)

Construct an `Any` by copying `value`.

## Any(NothingType)

Construct an empty `Any`.

## Any()

Identical to `Any(NothingType)`.

## Any(Any&&) and Any(const Any&)

Move or copy an `Any`, if the underlying type supports the respective operation.

If the underlying type does not support moving, the value will be copied instead. If the underlying type does not support copying, and a copy is requested, an exception is thrown.

---

# Methods

---

## is_a

Invoke: `is_a<T>()`

Returns: `bool`

Takes a template parameter `T`. Returns true if the held value has type `T`.

## get

Invoke: `get<T>()`

Returns: [`Maybe<T>`](maybe.md)

Takes a template parameter `T`. Returns a `Maybe<T>` containing the value if the value has type `T`. Otherwise, returns `Nothing`.

## when(closure)

Invoke: `when<T>(closure)`

Returns: `Maybe<U>`, where `U` is the return type of `closure`.

Takes a template parameter `T` and a generic closure/callback. If the held value is of type `T`, the closure is called with a reference to the held value.

Example:

    Any any { (int)123 };
    any.when<int>([](const int& number) {
      std::cout << "Any was an int: " << number << std::endl;
    });

## type_info

Invoke: `type_info()`

Returns: A pointer to a [TypeInfo](reflection.md#typeinfo).



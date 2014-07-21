# Class: Maybe

`Maybe<T>` is an option type that either holds a value of type `T` or `Nothing`.

The contents of a `Maybe<T>` can be accessed either with an "unsafe" API equivalent to `boost::optional` (or `std::optional` from C++14), or with a safer "monadic" interface that prevents errors through the type system.

If the contents of a `Maybe<T>` are accessed when the `Maybe<T>` contains `Nothing`, an exception is thrown of type `EmptyMaybeDereference`.

`Maybe<T&>` is specialized to support reference-like `Maybe`s, in which case they behave exactly like `Maybe<std::reference_wrapper<T>>`.

`Maybe<T>` is also specialized for all ["pointer-like" types](static.md#ispointerlike), so a `Maybe<std::unique_ptr<T>>` takes exactly the same memory as an `std::unique_ptr<T>`, and treats the NULL pointer as the "Nothing" value.

---

# Constructors

---

## Maybe(T value)

Construct a `Maybe<T>` containing `value`.

## Maybe(NothingType)

Construct an empty `Maybe`.

## Maybe(const Maybe<T>&)

Copy-constructor.

## Maybe(Maybe<T>&&)

Move-constructor.

---

# Methods

---

## get

Returns a point to the object. WARNING: Throws exception if the Maybe is empty.

## operator bool

Returns true if the Maybe has an object, otherwise false.

## operator->

Access the object as a pointer. WARNING: Throws exception if the Maybe is empty.

## operator*

Access the object by dereferencing. WARNING: Throws exception if the Maybe is empty.

## swap(Maybe<T>&)

Swaps the contents of this Maybe with another of the same type.

---

Functions

---

## Just

Invoke: `Just(x)`

Returns: `Maybe<T>`

Convenience function for constructing `Maybe<T>` on the go without typing out the full type name for the constructor.

## monad::fmap

Invoke: `fmap(maybe, closure)`

Returns: `Maybe<U>`, where `U` is the return type of `closure`.

The safer way to access the internals of a `Maybe<T>` — the closure will be invoked when the `Maybe<T>` contains a value, and the return value is another `Maybe` with the return value of the closure.

See also: [Monads/fmap](monads.md#monadfmap)

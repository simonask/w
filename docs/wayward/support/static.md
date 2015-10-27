# Static Metaprogramming

[wayward/support/meta.hpp](https://github.com/simonask/w/blob/master/wayward/support/meta.hpp)

These are mainly supplements to the type traits supplied by the standard `<type_traits>` header. Admittedly, some of them are slightly idiosynchratic (I prefer CamelCased types, while the Standard Library always goes for snake\_case). Nevertheless, these utilities are used throughout Wayward Support, and some of them are generally handy.

# Types

## TrueType, FalseType

Compile-time true/false.

## TypeList

Use: `TypeList<...>`

A list of types.

# Traits

## IsPointerLike

Use: `IsPointerLike<T>::Value`

True if `T` is a pointer-like type, i.e. if it is dereferenceable and nullable. Defaults to `std::is_pointer<T>::value`, but specialized by default for `std::unique_ptr<T>` and `std::shared_ptr<T>` (as well as `CloningPtr<T>`). This trait can be used to specialize container types that want to treat pointer-like types specially.


## Contains

Use: `Contains<T, TypeList<...>>::Value`

True if `TypeList<...>` contains `T`.

## IndexOf

Use: `IndexOf<T, TypeList<...>>::Value`

The index of `T` in `TypeList<...>`, or `SIZE_T_MAX` if it doesn't exist.

## RemoveConstRef

Use: `typedef RemoveConstRef<T>::Type`

Input    | Output
--------:|:------
T        | T
T&       | T
const T& | T

## MaxSize

Use: `MaxSize<TypeList<...>>::Value`

`Value` is the maximum size of all the types in the `TypeList<...>`.

## MaxAlignment

Use: `MaxAlignment<TypeList<...>>::Value`

`Value` is the maximum alignment of all the types in the `TypeList<...>`.

## AreAllCopyConstructible

Use: `AreAllCopyConstructible<TypeList<...>>::Value`

True if all types in the `TypeList<...>` are copy-constructible.

## AreAllCopyAssignable

Use: `AreAllCopyAssignable<TypeList<...>>::Value`

True if all types in the `TypeList<...>` are copy-assignable.

## AreAllMoveConstructible

Use: `AreAllMoveConstructible<TypeList<...>>::Value`

True if all types in the `TypeList<...>` are move-constructible.

## AreAllMoveAssignable

Use: `AreAllMoveAssignable<TypeList<...>>::Value`

True if all types in the `TypeList<...>` are move-assignable.

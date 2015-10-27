# Class: Fiber

[wayward/support/fiber.hpp](https://github.com/simonask/w/blob/master/wayward/support/fiber.hpp)

Fiber is an implementation of coroutines. It uses `setjmp`/`longjmp` internally to manage execution contexts, but supports exceptions and stack unwinding.

---

# Types

---

## FiberPtr

An opaque pointer with value semantics representing a reference to a fiber/coroutine.

It is currently defined in terms of `std::shared_ptr`. When the last reference to a fiber is destroyed, the fiber is terminated and destroyed as well.

## fiber::Function

Defined as `std::function<void()>` — The entry point of a fiber.

## fiber::ErrorHandler

Defined as `std::function<void(std::exception_ptr)>` — A callback that will be called if an exception is uncaught and the stack unwinds beyond a fiber boundary.

---

# Functions

---

## fiber::current

Returns: A `FiberPtr` representing the currently running fiber. If a thread has not launched any fibers yet, a special `FiberPtr` representing the thread is returned. There is no difference between this and regular fibers, except all threads including the main thread always have an implicit reference to the main thread fiber, so it never goes out of scope before the thread finishes.

## fiber::create

Invoke: `fiber::create(Function)` or `fiber::create(Function, ErrorHandler)`

Create a new fiber (optionally with an error handler) without starting it.

If an [`ErrorHandler`](#fiberErrorHandler) is provided, it will be called if the fiber throws an exception that remains uncaught at the fiber boundary. The default error handler terminates the program (similar to how an uncaught exception outside of fibers behave).

## fiber::resume

Invoke: `fiber::resume(FiberPtr)`

Start or resume a fiber.

## fiber::create

Invoke: `fiber::start(Function)`

Equivalent to [`fiber::create`](#fibercreate) + [`fiber::resume`](#fiberresume)

## fiber::terminate

Invoke: `fiber::terminate(FiberPtr)`

Terminates a fiber, unwinding its stack and running all destructors.

Internally, this resumes the fiber with a signal that tells the fiber to throw a special exception handled silently at the fiber boundary before termination.

## fiber::yield

Invoke: `fiber::yield()`

Resumes the fiber that resumed the current fiber. Throws an exception if the current fiber is orphaned (i.e., was never resumed by anybody else, i.e. is most likely the main fiber).

#pragma once
#ifndef WAYWARD_SUPPORT_FIBER_HPP_INCLUDED
#define WAYWARD_SUPPORT_FIBER_HPP_INCLUDED

#include <memory>
#include <functional>
#include <tuple>

#include <wayward/support/error.hpp>
#include <wayward/support/datetime.hpp>

namespace wayward {
  struct FiberError : Error {
    FiberError(const std::string& msg) : Error(msg) {}
  };

  struct FiberTermination {};
  struct Fiber;

  /*
    Fiber is implemented in terms of std::shared_ptr. A running fiber has a
    reference to the fiber that invoked it.
  */
  using FiberPtr = std::shared_ptr<Fiber>;

  namespace fiber {
    using Function = std::function<void()>;
    using ErrorHandler = std::function<void(std::exception_ptr)>;

    FiberPtr current();

    /*
      Create a new fiber without starting it.
    */
    FiberPtr create(Function);

    /*
      Create a new fiber with an error handler, without starting it.
    */
    FiberPtr create(Function, ErrorHandler);

    /*
      Resume a fiber (or start it if it hasn't been started yet).
    */
    void resume(FiberPtr);

    /*
      Create a fiber and start it right away.
    */
    void start(Function);

    /*
      Resume the fiber with a signal to terminate.
    */
    void terminate(FiberPtr);

    /*
      Resume the fiber that started the current fiber.
      Throws an exception if the current fiber is orphaned.
    */
    void yield();
  }
}

#endif // WAYWARD_SUPPORT_FIBER_HPP_INCLUDED

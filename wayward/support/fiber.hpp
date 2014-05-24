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

  struct Fiber {
    static Fiber& current();

    using Function = std::function<void()>;
    using ErrorHandler = std::function<void(std::exception_ptr)>;

    explicit Fiber(Function function);
    Fiber(Function function, ErrorHandler error_handler);
    ~Fiber();

    /*
      If the fiber throws an uncaught exception, the error handler function will be called.
      If no error handler is set, std::abort() is called.
    */
    void set_error_handler(ErrorHandler);

    /*
      The fiber 'new_owner' will be resumed upon natural return from the fiber function.
      By default, the fiber that *started* this fiber will be resumed upon return.
      NOTE: That fiber may be a different one from the one that last *resumed* the fiber.
    */
    void set_on_exit(Fiber* new_owner);


    /*
      Resume the fiber (or start it if it hasn't run yet).
    */
    void resume();
    void operator()() { resume(); }

    /*
      terminate resumes the fiber with a signal that instructs it to unwind its stack,
      before in turn resuming the caller of terminate.
    */
    void terminate();

    struct Private;
    std::unique_ptr<Private> p_;
  private:
    Fiber();
  };

  // NOTE: These functions require an event loop!
  void yield();
  void sleep(DateTimeInterval);
}

#endif // WAYWARD_SUPPORT_FIBER_HPP_INCLUDED

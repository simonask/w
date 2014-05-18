#pragma once
#ifndef WAYWARD_SUPPORT_FIBER_HPP_INCLUDED
#define WAYWARD_SUPPORT_FIBER_HPP_INCLUDED

#include <memory>
#include <functional>
#include <tuple>

#include <wayward/support/error.hpp>

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

    void set_error_handler(ErrorHandler);
    void set_on_exit(Fiber* new_owner);
    void operator()() { resume(); }
    void resume();
    void terminate();

    struct Private;
    std::unique_ptr<Private> p_;
  private:
    Fiber();
  };
}

#endif // WAYWARD_SUPPORT_FIBER_HPP_INCLUDED

#pragma once
#ifndef WAYWARD_SUPPORT_EVENT_LOOP_HPP_INCLUDED
#define WAYWARD_SUPPORT_EVENT_LOOP_HPP_INCLUDED

#include <functional>

#include <wayward/support/datetime.hpp>

namespace wayward {
  using FDEvents = uint64_t;

  enum class FDEvent : uint64_t {
    Read    = 1,
    Write   = 1 << 1,
    Accept  = 1 << 2,
    Timeout = 1 << 3,
    All     = UINT64_T_MAX
  };

  struct IEventHandle {
    virtual ~IEventHandle() {}
  };

  struct IEventLoop {
    using FDEventCallback = std::function<void(int, FDEvents events)>;

    virtual ~IEventLoop() {}
    virtual void run() = 0;
    virtual void resume() = 0;
    virtual void terminate() = 0;
    virtual void* native_handle() const = 0;

    virtual std::unique_ptr<IEventHandle>
    add_file_descriptor(int fd, FDEvents events, FDEventCallback callback) = 0;

    virtual std::unique_ptr<IEventHandle>
    call_at(DateTime time, std::function<void()> callback) = 0;
  };

  IEventLoop* current_event_loop();
  void set_current_event_loop(IEventLoop* loop);

  namespace fiber {
    void yield();
    void sleep(DateTimeInterval interval);
  }
}

#endif // WAYWARD_SUPPORT_EVENT_LOOP_HPP_INCLUDED

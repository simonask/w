#pragma once
#ifndef WAYWARD_SUPPORT_EVENT_LOOP_HPP_INCLUDED
#define WAYWARD_SUPPORT_EVENT_LOOP_HPP_INCLUDED

#include <functional>

#include <wayward/support/datetime.hpp>

namespace wayward {
  using FDEvents = uint64_t;

  enum class FDEvent : int16_t {
    // These values are binary compatible with libevent.
    Timeout = 0x01,
    Read    = 0x02,
    Write   = 0x04,
    Signal  = 0x08,
    Closed  = 0x80,
    All     = 0xff
  };

  struct IEventHandle {
    virtual ~IEventHandle() {}
  };

  struct IEventLoop {
    using FDEventCallback = std::function<void(int, int64_t events)>;

    virtual ~IEventLoop() {}
    virtual void run() = 0;
    virtual void* native_handle() const = 0;

    virtual std::unique_ptr<IEventHandle>
    add_file_descriptor(int fd, FDEvents events, FDEventCallback callback) = 0;

    virtual std::unique_ptr<IEventHandle>
    call_in(DateTimeInterval interval, std::function<void()> callback, bool repeat = false) = 0;
  };

  IEventLoop* current_event_loop();
  void set_current_event_loop(IEventLoop* loop);

  struct EventLoop : IEventLoop {
    EventLoop();
    virtual ~EventLoop();

    void run() final;
    void* native_handle() const;

    std::unique_ptr<IEventHandle>
    add_file_descriptor(int fd, FDEvents events, FDEventCallback callback) final;

    std::unique_ptr<IEventHandle>
    call_in(DateTimeInterval interval, std::function<void()> callback, bool repeat = false) final;

    explicit EventLoop(void* native_handle); // Only for internal use!
  private:
    struct Private;
    std::unique_ptr<Private> p_;
  };
}

#endif // WAYWARD_SUPPORT_EVENT_LOOP_HPP_INCLUDED

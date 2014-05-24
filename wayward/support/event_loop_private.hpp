#pragma once
#ifndef WAYWARD_SUPPORT_EVENT_LOOP_PRIVATE_HPP_INCLUDED
#define WAYWARD_SUPPORT_EVENT_LOOP_PRIVATE_HPP_INCLUDED

#include <event2/event.h>
#include <event2/http.h>
#include <event2/keyvalq_struct.h>
#include <event2/buffer.h>
#include <wayward/support/event_loop.hpp>

namespace wayward {
  namespace detail {
    inline event_base* current_libevent_base() {
      return static_cast<event_base*>(wayward::current_event_loop()->native_handle());
    }
  }
}

#endif // WAYWARD_SUPPORT_EVENT_LOOP_PRIVATE_HPP_INCLUDED

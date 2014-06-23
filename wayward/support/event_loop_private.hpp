#pragma once
#ifndef WAYWARD_SUPPORT_EVENT_LOOP_PRIVATE_INCLUDED
#define WAYWARD_SUPPORT_EVENT_LOOP_PRIVATE_INCLUDED

#include <wayward/support/event_loop.hpp>

#include <event2/event.h>

namespace wayward {
  void set_current_event_loop(IEventLoop* transient_loop);
}

#endif // WAYWARD_SUPPORT_EVENT_LOOP_PRIVATE_INCLUDED

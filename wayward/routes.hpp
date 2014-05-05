#pragma once
#ifndef WAYWARD_ROUTES_HPP_INCLUDED
#define WAYWARD_ROUTES_HPP_INCLUDED

#include <wayward/w.hpp>

#include <persistence/context.hpp>
#include <persistence/projection.hpp>

namespace wayward {
  struct Routes {
    // Override these to make before- and after-filters:
    virtual void before(Request&) {}
    virtual void after(Request&) {}
    virtual Response around(Request& req, std::function<Response(Request&)> yield) { return yield(req); }

    persistence::Context persistence_context;

    template <typename Type>
    persistence::Projection<Type> from() {
      return persistence::from<Type>(persistence_context);
    }
  };
}

#endif // WAYWARD_ROUTES_HPP_INCLUDED

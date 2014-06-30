#pragma once
#ifndef WAYWARD_ROUTES_HPP_INCLUDED
#define WAYWARD_ROUTES_HPP_INCLUDED

#include <wayward/w.hpp>

#include <persistence/context.hpp>
#include <persistence/create.hpp>
#include <persistence/destroy.hpp>
#include <persistence/projection.hpp>

namespace wayward {
  struct Routes {
    // Override these to make before- and after-filters:
    virtual void before(Request&) {}
    virtual void after(Request&) {}
    virtual Response around(Request& req, std::function<Response(Request&)> yield) { return yield(req); }

    Session session;

    persistence::Context persistence_context;

    template <typename Type>
    persistence::Projection<Type> from() {
      return persistence::from<Type>(persistence_context);
    }

    template <typename Type>
    persistence::RecordPtr<Type> create(const data_franca::Spectator& data) {
      return persistence::create<Type>(persistence_context, data);
    }

    template <typename Type>
    bool destroy(persistence::RecordPtr<Type>& ptr) {
      return persistence::destroy(persistence_context, ptr);
    }
  };
}

#endif // WAYWARD_ROUTES_HPP_INCLUDED

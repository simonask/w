#pragma once
#ifndef PERSISTENCE_DESTROY_HPP_INCLUDED
#define PERSISTENCE_DESTROY_HPP_INCLUDED

#include <persistence/record_ptr.hpp>
#include <persistence/context.hpp>

namespace persistence {
  template <typename T>
  bool destroy(Context& ctx, RecordPtr<T>& ptr) {
    return false; // TODO
  }
}

#endif // PERSISTENCE_DESTROY_HPP_INCLUDED

#pragma once
#ifndef PERSISTENCE_TYPE_HPP_INCLUDED
#define PERSISTENCE_TYPE_HPP_INCLUDED

#include <string>

namespace persistence {
  struct IType {
    virtual std::string name() const = 0;
    virtual bool is_nullable() const = 0;
  };

  template <typename T> struct BuildType;
  template <typename T>
  auto get_type() -> const decltype(BuildType<T>::build()) {
    static const auto t = BuildType<T>::build();
    return t;
  }
}

#endif // PERSISTENCE_TYPE_HPP_INCLUDED

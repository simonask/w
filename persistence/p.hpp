#pragma once
#ifndef P_HPP_INCLUDED
#define P_HPP_INCLUDED

#include <persistence/type.hpp>
#include <persistence/belongs_to.hpp>
#include <persistence/has_many.hpp>
#include <persistence/has_one.hpp>
#include <persistence/record_type.hpp>
#include <persistence/record_type_builder.hpp>
#include <persistence/persistence_macro.hpp>
#include <persistence/primary_key.hpp>
#include <persistence/connection.hpp>
#include <persistence/relational_algebra.hpp>

#if !defined(PERSISTENCE_NO_SHORTHAND_NAMESPACE)
namespace p = persistence;
#endif

namespace persistence {
  struct Configuration {
    std::string connection_string;
    size_t pool_size;
  };

  bool connect(const Configuration& config, std::string* out_error = nullptr);
  IConnection& get_connection();
}

#endif

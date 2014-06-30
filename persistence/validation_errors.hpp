#pragma once
#ifndef PERSISTENCE_VALIDATION_ERRORS_HPP_INCLUDED
#define PERSISTENCE_VALIDATION_ERRORS_HPP_INCLUDED

#include <wayward/support/error.hpp>

namespace persistence {
  struct ValidationErrors : wayward::Error {
    ValidationErrors(const std::string& msg) : wayward::Error(msg) {}
    // TODO
  };
}

namespace wayward {
  namespace data_franca {
    template <>
    struct Adapter<persistence::ValidationErrors> : AdapterBase<persistence::ValidationErrors> {
      Adapter(persistence::ValidationErrors& ref, Bitflags<Options> opts) : AdapterBase<persistence::ValidationErrors>(ref, opts) {}
    };
  }
}

#endif // PERSISTENCE_VALIDATION_ERRORS_HPP_INCLUDED

#pragma once
#ifndef WAYWARD_SUPPORT_DATA_FRANCA_TYPES_HPP_INCLUDED
#define WAYWARD_SUPPORT_DATA_FRANCA_TYPES_HPP_INCLUDED

#include <string>
#include <memory>

namespace wayward {
  namespace data_franca {
    using Boolean = bool;
    using Integer = std::int64_t;
    using Real    = double;
    using String  = std::string;

    enum class DataType {
      Nothing,
      Boolean,
      Integer,
      Real,
      String,
      List,
      Dictionary,
    };
  }
}

#endif // WAYWARD_SUPPORT_DATA_FRANCA_TYPES_HPP_INCLUDED

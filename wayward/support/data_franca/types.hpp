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

    struct IReader;
    struct IWriter;
    struct IAdapter;
    using ReaderPtr = std::shared_ptr<const IReader>;
    using WriterPtr = std::shared_ptr<IWriter>;
    using AdapterPtr = std::shared_ptr<IAdapter>;
  }
}

#endif // WAYWARD_SUPPORT_DATA_FRANCA_TYPES_HPP_INCLUDED

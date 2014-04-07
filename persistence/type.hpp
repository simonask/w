#pragma once
#ifndef PERSISTENCE_TYPE_HPP_INCLUDED
#define PERSISTENCE_TYPE_HPP_INCLUDED

#include <string>

namespace persistence {
  struct IType {
    virtual std::string name() const = 0;
    virtual bool is_nullable() const = 0;
  };

  struct IResultSet;

  template <typename T>
  struct IDataTypeFor : IType {
    virtual void extract_from_results(T& value, const IResultSet&, size_t row, const std::string& col_name) const = 0;
    virtual bool has_value(const T& value) const = 0;
  };

  template <typename T>
  struct TypeIdentifier { TypeIdentifier() {} };

  // Implement this:
  // const IType* build_type(const TypeIdentifier<T>* null);

  template <typename T>
  auto get_type() -> decltype(build_type(std::declval<const TypeIdentifier<T>*>())) {
    static const TypeIdentifier<T> type_id;
    static const auto t = build_type(&type_id);
    return t;
  }
}

#endif // PERSISTENCE_TYPE_HPP_INCLUDED

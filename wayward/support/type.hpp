#pragma once
#ifndef WAYWARD_SUPPORT_TYPE_HPP_INCLUDED
#define WAYWARD_SUPPORT_TYPE_HPP_INCLUDED

#include <wayward/support/type_info.hpp>

namespace wayward {
  struct IType {
    virtual std::string name() const = 0;
    virtual bool is_nullable() const = 0;
    virtual const TypeInfo& type_info() const = 0;
  };

  namespace data_franca {
    struct ScalarSpectator;
    struct ScalarMutator;
  }

  template <class T, class Base = IType>
  struct IDataTypeFor : Base {
    virtual bool deserialize_value(T& value, const wayward::data_franca::ScalarSpectator& source) const = 0;
    virtual bool serialize_value(const T& value, wayward::data_franca::ScalarMutator& target) const = 0;
    virtual bool has_value(const T& value) const = 0;

    const TypeInfo& type_info() const override { return GetTypeInfo<T>::Value; }
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

#endif // WAYWARD_SUPPORT_TYPE_HPP_INCLUDED

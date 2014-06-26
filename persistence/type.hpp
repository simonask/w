#pragma once
#ifndef PERSISTENCE_TYPE_HPP_INCLUDED
#define PERSISTENCE_TYPE_HPP_INCLUDED

#include <string>

#include <wayward/support/type_info.hpp>
#include <wayward/support/result.hpp>
#include <wayward/support/any.hpp>
#include <wayward/support/meta.hpp>

#include <persistence/ast.hpp>

namespace wayward {
  namespace data_franca {
    struct ScalarSpectator;
    struct ScalarMutator;
  }
}

namespace persistence {
  using wayward::TypeInfo;
  using wayward::Result;
  using wayward::AnyRef;
  using wayward::AnyConstRef;
  using wayward::Success;

  struct IType {
    virtual std::string name() const = 0;
    virtual bool is_nullable() const = 0;
    virtual const TypeInfo& type_info() const = 0;
  };

  struct ISerializableType : IType {
    virtual Result<void> deserialize_data(AnyRef data, const wayward::data_franca::ScalarSpectator& source) const = 0;
    virtual Result<void> serialize_data(AnyConstRef data, wayward::data_franca::ScalarMutator& target) const = 0;
  };

  struct ISQLType : ISerializableType {
    virtual ast::Ptr<ast::SingleValue> make_literal(AnyConstRef data) const = 0;
  };

  struct IResultSet;

  template <class T, class Base = ISQLType>
  struct IDataTypeFor : Base {
    virtual Result<void> deserialize_value(T& value, const wayward::data_franca::ScalarSpectator& source) const = 0;
    virtual Result<void> serialize_value(const T& value, wayward::data_franca::ScalarMutator& target) const = 0;
    virtual bool has_value(const T& value) const = 0;
  };

  template <class T, class Base = ISQLType>
  struct DataTypeFor : IDataTypeFor<T, Base> {
    Result<void> deserialize_data(AnyRef data, const wayward::data_franca::ScalarSpectator& source) const final {
      return this->deserialize_value(*data.get<T&>(), source);
    }
    Result<void> serialize_data(AnyConstRef data, wayward::data_franca::ScalarMutator& target) const final {
      return this->serialize_value(*data.get<const T&>(), target);
    }
    const TypeInfo& type_info() const final {
      return wayward::GetTypeInfo<T>::Value;
    }
  };

  template <typename T>
  struct TypeIdentifier { TypeIdentifier() {} };

  // Implement this:
  // const IType* build_type(const TypeIdentifier<T>* null);

  template <typename T>
  auto get_type() -> decltype(build_type(std::declval<const TypeIdentifier<typename wayward::meta::RemoveConstRef<T>::Type>*>())) {
    static const TypeIdentifier<typename wayward::meta::RemoveConstRef<T>::Type> type_id;
    static const auto t = build_type(&type_id);
    return t;
  }
}

#endif // PERSISTENCE_TYPE_HPP_INCLUDED

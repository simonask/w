#pragma once
#ifndef WAYWARD_SUPPORT_TYPE_HPP_INCLUDED
#define WAYWARD_SUPPORT_TYPE_HPP_INCLUDED

#include <wayward/support/type_info.hpp>
#include <wayward/support/format.hpp>
#include <wayward/support/error.hpp>
#include <wayward/support/meta.hpp>
#include <wayward/support/any.hpp>

namespace wayward {
  struct DataVisitor;

  struct TypeError : Error {
    TypeError(const std::string& msg) : Error(msg) {}
  };

  struct IType {
    virtual std::string name() const = 0;
    virtual bool is_nullable() const = 0;
    virtual const TypeInfo& type_info() const = 0;

    virtual void visit_data(AnyConstRef data, DataVisitor& visitor) const = 0;
  };

  template <class T, class Base = IType>
  struct IDataTypeFor : Base {
    virtual void visit(T& value, DataVisitor& visitor) const = 0;

    virtual bool has_value(const T& value) const = 0;
  };

  template <class T, class Base = IType>
  struct DataTypeFor : IDataTypeFor<T, Base> {
    const TypeInfo& type_info() const final {
      return GetTypeInfo<T>::Value;
    }

    void visit_data(AnyConstRef data, DataVisitor& visitor) const final {
      if (&data.type_info() != &this->type_info()) {
        throw TypeError(format("visit_data called with data of wrong type (got {0}, expected {1}).", data.type_info().name(), this->name()));
      }
      // XXX: Erm.. Maybe we should find a better way to do this.
      this->visit(const_cast<T&>(*data.get<const T&>()), visitor);
    }
  };

  template <typename T>
  struct TypeIdentifier { TypeIdentifier() {} };

  // Implement this:
  // const IType* build_type(const TypeIdentifier<T>* null);

  template <typename T>
  auto get_type() -> decltype(build_type(std::declval<const TypeIdentifier<typename meta::RemoveConstRef<T>::Type>*>())) {
    static const TypeIdentifier<typename meta::RemoveConstRef<T>::Type> type_id;
    static const auto t = build_type(&type_id);
    return t;
  }
}

#endif // WAYWARD_SUPPORT_TYPE_HPP_INCLUDED

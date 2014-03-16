#pragma once
#ifndef PERSISTENCE_PROPERTY_HPP_INCLUDED
#define PERSISTENCE_PROPERTY_HPP_INCLUDED

#include <string>
#include <persistence/type.hpp>

namespace persistence {
  struct IProperty {
    virtual ~IProperty() {}
    virtual std::string column() const = 0;
    virtual const IType& type() const = 0;
  };

  template <typename T>
  struct Property {
    explicit Property(std::string column) : column_(std::move(column)) {}

  protected:
    std::string column_;
  };

  template <typename T>
  struct IPropertyOf : IProperty {
    virtual ~IPropertyOf() {}
  };

  template <typename M, typename T>
  struct PropertyOf : IPropertyOf<M>, Property<T> {
    explicit PropertyOf(std::string column) : Property<T>{column} {}
    const IType& type() const { return *get_type<T>(); }
    std::string column() const { return this->column_; }
  };
}

#endif // PERSISTENCE_PROPERTY_HPP_INCLUDED

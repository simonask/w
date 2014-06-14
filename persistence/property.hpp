#pragma once
#ifndef PERSISTENCE_PROPERTY_HPP_INCLUDED
#define PERSISTENCE_PROPERTY_HPP_INCLUDED

#include <string>
#include <persistence/type.hpp>
#include <persistence/result_set.hpp>
#include <persistence/ast.hpp>
#include <persistence/data_ref.hpp>

#include <wayward/support/cloning_ptr.hpp>
#include <wayward/support/data_franca/adapter.hpp>
#include <wayward/support/data_franca/spectator.hpp>
#include <wayward/support/data_franca/mutator.hpp>

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

  namespace ast {
    struct SingleValue;
  }

  template <typename T>
  struct IPropertyOf : IProperty {
    virtual ~IPropertyOf() {}

    virtual bool has_value(const T& record) const = 0;
    virtual bool deserialize(T& record, const wayward::data_franca::ScalarSpectator& value) const = 0;
    virtual bool serialize(const T& record, wayward::data_franca::ScalarMutator& target) const = 0;

    virtual wayward::data_franca::ReaderPtr
    get_member_reader(const T&, wayward::Bitflags<wayward::data_franca::Options> options) const = 0;

    virtual wayward::data_franca::AdapterPtr
    get_member_adapter(T&, wayward::Bitflags<wayward::data_franca::Options> options) const = 0;

    virtual DataRef
    get_data(const T&) const = 0;
  };

  struct ASTError : wayward::Error {
    ASTError(const std::string& msg) : wayward::Error(msg) {}
  };

  template <typename T, typename M>
  struct PropertyOfBase : IPropertyOf<T>, Property<M> {
    using MemberPtr = M T::*;
    MemberPtr ptr_;
    PropertyOfBase(MemberPtr ptr, std::string column) : Property<M>{column}, ptr_(ptr) {}
    const IType& type() const { return *get_type<M>(); }
    std::string column() const { return this->column_; }

    const M& get(const T& record) const {
      return record.*ptr_;
    }

    M& get(T& record) const {
      return record.*ptr_;
    }

    bool has_value(const T& record) const override {
      return get_type<M>()->has_value(get(record));
    }

    bool deserialize(T& record, const wayward::data_franca::ScalarSpectator& value) const override {
      return get_type<M>()->deserialize_value(get(record), value);
    }

    bool serialize(const T& record, wayward::data_franca::ScalarMutator& target) const override {
      return get_type<M>()->serialize_value(get(record), target);
    }

    wayward::data_franca::ReaderPtr
    get_member_reader(const T& object, wayward::Bitflags<wayward::data_franca::Options> options) const override {
      return wayward::data_franca::make_reader(get(object), options);
    }

    wayward::data_franca::AdapterPtr
    get_member_adapter(T& object, wayward::Bitflags<wayward::data_franca::Options> options) const override {
      return wayward::data_franca::make_adapter(get(object), options);
    }

    DataRef
    get_data(const T& object) const override {
      return DataRef{get(object)};
    }
  };

  template <typename T, typename M>
  struct PropertyOf : PropertyOfBase<T, M> {
    using MemberPtr = typename PropertyOfBase<T, M>::MemberPtr;
    PropertyOf(MemberPtr ptr, std::string col) : PropertyOfBase<T, M>(ptr, std::move(col)) {}
  };
}

#endif // PERSISTENCE_PROPERTY_HPP_INCLUDED

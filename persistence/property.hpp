#pragma once
#ifndef PERSISTENCE_PROPERTY_HPP_INCLUDED
#define PERSISTENCE_PROPERTY_HPP_INCLUDED

#include <string>
#include <wayward/support/type.hpp>
#include <persistence/result_set.hpp>
#include <persistence/ast.hpp>

#include <wayward/support/result.hpp>
#include <wayward/support/any.hpp>
#include <wayward/support/cloning_ptr.hpp>
#include <wayward/support/data_franca/adapter.hpp>
#include <wayward/support/data_franca/spectator.hpp>
#include <wayward/support/data_franca/mutator.hpp>

namespace persistence {
  using wayward::Result;
  using wayward::Any;
  using wayward::AnyRef;
  using wayward::AnyConstRef;
  using wayward::Nothing;
  using wayward::IType;
  using wayward::TypeInfo;
  using wayward::get_type;

  struct IProperty {
    virtual ~IProperty() {}
    virtual std::string column() const = 0;
    virtual const IType& type() const = 0;

    virtual Result<Any> get(AnyConstRef record) const = 0;
    virtual Result<void> set(AnyRef record, AnyConstRef value) const = 0;
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

    virtual wayward::data_franca::ReaderPtr
    get_member_reader(const T&, wayward::Bitflags<wayward::data_franca::Options> options) const = 0;

    virtual wayward::data_franca::AdapterPtr
    get_member_adapter(T&, wayward::Bitflags<wayward::data_franca::Options> options) const = 0;

    virtual void
    visit(T&, wayward::DataVisitor& visitor) const = 0;
  };

  struct ASTError : wayward::Error {
    ASTError(const std::string& msg) : wayward::Error(msg) {}
  };

  struct TypeError : wayward::Error {
    TypeError(const std::string& msg) : wayward::Error(msg) {}
  };

  namespace detail {
    wayward::ErrorPtr make_type_error_for_mismatching_record_type(const IType* expected_type, const TypeInfo& got_type);
    wayward::ErrorPtr make_type_error_for_mismatching_value_type(const IType* record_type, const IType* expected_type, const TypeInfo& got_type);
  }

  template <typename T, typename M>
  struct PropertyOfBase : IPropertyOf<T>, Property<M> {
    using MemberPtr = M T::*;
    MemberPtr ptr_;
    PropertyOfBase(MemberPtr ptr, std::string column) : Property<M>{column}, ptr_(ptr) {}
    const IType& type() const { return *wayward::get_type<M>(); }
    std::string column() const { return this->column_; }

    void visit(T& record, wayward::DataVisitor& visitor) const final {
      visitor[this->column()](get_known(record));
    }

    Result<Any> get(AnyConstRef record) const override {
      if (!record.is_a<T>()) {
        return detail::make_type_error_for_mismatching_record_type(get_type<T>(), record.type_info());
      }
      auto& mref = *record.get<const T&>();
      return Any{ get_known(mref) };
    }

    Result<void> set(AnyRef record, AnyConstRef value) const override {
      if (!record.is_a<T>()) {
        return detail::make_type_error_for_mismatching_record_type(get_type<T>(), record.type_info());
      }
      if (!value.is_a<M>()) {
        return detail::make_type_error_for_mismatching_value_type(get_type<T>(), get_type<M>(), value.type_info());
      }
      auto& mref = *record.get<T&>();
      auto vref = value.get<M>();
      get_known(mref) = *vref;
      return Nothing;
    }

    const M& get_known(const T& record) const {
      return record.*ptr_;
    }

    M& get_known(T& record) const {
      return record.*ptr_;
    }

    bool has_value(const T& record) const override {
      return get_type<M>()->has_value(get_known(record));
    }

    wayward::data_franca::ReaderPtr
    get_member_reader(const T& object, wayward::Bitflags<wayward::data_franca::Options> options) const override {
      return wayward::data_franca::make_reader(get_known(object), options);
    }

    wayward::data_franca::AdapterPtr
    get_member_adapter(T& object, wayward::Bitflags<wayward::data_franca::Options> options) const override {
      return wayward::data_franca::make_adapter(get_known(object), options);
    }
  };

  template <typename T, typename M>
  struct PropertyOf : PropertyOfBase<T, M> {
    using MemberPtr = typename PropertyOfBase<T, M>::MemberPtr;
    PropertyOf(MemberPtr ptr, std::string col) : PropertyOfBase<T, M>(ptr, std::move(col)) {}
  };
}

#endif // PERSISTENCE_PROPERTY_HPP_INCLUDED

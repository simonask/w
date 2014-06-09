#pragma once
#ifndef PERSISTENCE_PROPERTY_HPP_INCLUDED
#define PERSISTENCE_PROPERTY_HPP_INCLUDED

#include <string>
#include <persistence/type.hpp>
#include <persistence/result_set.hpp>

#include <wayward/support/data_franca/adapter.hpp>

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

    virtual bool has_value(const T& record) const = 0;
    virtual void set(T& record, const IResultSet&, size_t row_num, const std::string& col_name) const = 0;

    virtual wayward::data_franca::ReaderPtr
    get_member_reader(const T&) const = 0;

    virtual wayward::data_franca::AdapterPtr
    get_member_adapter(T&) const = 0;
  };

  template <typename T, typename M>
  struct PropertyOf : IPropertyOf<T>, Property<M> {
    using MemberPtr = M T::*;
    MemberPtr ptr_;
    explicit PropertyOf(MemberPtr ptr, std::string column) : Property<M>{column}, ptr_(ptr) {}
    const IType& type() const { return *get_type<M>(); }
    std::string column() const { return this->column_; }

    bool has_value(const T& record) const override {
      auto& value = record.*ptr_;
      return get_type<M>()->has_value(value);
    }

    void set(T& record, const IResultSet& results, size_t row_num, const std::string& col_name) const override {
      M* value_ptr = &(record.*ptr_);
      get_type<M>()->extract_from_results(*value_ptr, results, row_num, col_name);
    };

    wayward::data_franca::ReaderPtr
    get_member_reader(const T& object) const override {
      const M* value_ptr = &(object.*ptr_);
      return wayward::data_franca::make_reader(*value_ptr);
    }

    wayward::data_franca::AdapterPtr
    get_member_adapter(T& object) const override {
      M* value_ptr = &(object.*ptr_);
      return wayward::data_franca::make_adapter(*value_ptr);
    }
  };
}

#endif // PERSISTENCE_PROPERTY_HPP_INCLUDED

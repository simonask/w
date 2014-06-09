#pragma once
#ifndef PERSISTENCE_TYPES_HPP_INCLUDED
#define PERSISTENCE_TYPES_HPP_INCLUDED

#include <string>
#include <cstdint>
#include <sstream>

#include <persistence/type.hpp>
#include <persistence/result_set.hpp>
#include <wayward/support/maybe.hpp>
#include <wayward/support/data_franca/mutator.hpp>
#include <wayward/support/data_franca/spelunker.hpp>

namespace persistence {
  struct StringType : IDataTypeFor<std::string> {
    bool is_nullable() const final { return false; }
    std::string name() const final { return "std::string"; }

    bool has_value(const std::string& value) const final {
      return true;
    }

    bool deserialize_value(std::string& value, const wayward::data_franca::ScalarSpelunker& input) const override {
      return input >> value;
    }

    bool serialize_value(const std::string& value, wayward::data_franca::ScalarMutator& target) const override {
      target << value;
      return true;
    }
  };

  const StringType* build_type(const TypeIdentifier<std::string>*);

  template <typename T>
  struct NumericType : IDataTypeFor<T> {
    NumericType(std::string name) : name_(std::move(name)) {}
    bool is_nullable() const final { return false; }
    std::string name() const final { return name_; }
    size_t bits() const { return sizeof(T)*8; }
    bool is_signed() const { return std::is_signed<T>::value; }
    bool is_float() const { return std::is_floating_point<T>::value; }

    bool has_value(const T& value) const final { return true; }

    bool deserialize_value(T& value, const wayward::data_franca::ScalarSpelunker& source) const override {
      if (is_float()) {
        wayward::data_franca::Real r;
        if (source >> r) {
          value = static_cast<T>(r);
          return true;
        }
      } else {
        wayward::data_franca::Integer n;
        if (source >> n) {
          value = static_cast<T>(n);
          return true;
        }
      }
      return false;
    }

    bool serialize_value(const T& value, wayward::data_franca::ScalarMutator& target) const override {
      if (is_float()) {
        target << static_cast<wayward::data_franca::Real>(value);
      } else {
        target << static_cast<wayward::data_franca::Integer>(value);
      }
      return true;
    }
  private:
    std::string name_;
  };

  const NumericType<std::int32_t>* build_type(const TypeIdentifier<std::int32_t>*);
  const NumericType<std::int64_t>* build_type(const TypeIdentifier<std::int64_t>*);
  const NumericType<std::uint32_t>* build_type(const TypeIdentifier<std::uint32_t>*);
  const NumericType<std::uint64_t>* build_type(const TypeIdentifier<std::uint64_t>*);
  const NumericType<float>* build_type(const TypeIdentifier<float>*);
  const NumericType<double>* build_type(const TypeIdentifier<double>*);

  namespace detail {
    std::string maybe_type_name(const IType* inner);
  }

  template <typename T>
  struct MaybeType : IDataTypeFor<wayward::Maybe<T>> {
    MaybeType(const IDataTypeFor<T>* inner_type) : inner_type_(inner_type) {}
    std::string name() const final { return detail::maybe_type_name(inner_type_); }
    bool is_nullable() const { return true; }

    bool has_value(const wayward::Maybe<T>& value) const final {
      return static_cast<bool>(value);
    }

    bool deserialize_value(wayward::Maybe<T>& value, const wayward::data_franca::ScalarSpelunker& source) const final {
      if (source) {
        T val;
        inner_type_->deserialize_value(val, source);
        value = std::move(val);
      } else {
        value = wayward::Nothing;
      }
      return true;
    }

    bool serialize_value(const wayward::Maybe<T>& value, wayward::data_franca::ScalarMutator& target) const final {
      if (value) {
        inner_type_->serialize_value(*value, target);
      } else {
        target << wayward::Nothing;
      }
      return true;
    }
  private:
    const IDataTypeFor<T>* inner_type_ = nullptr;
  };

  template <typename T>
  const MaybeType<T>* build_type(const TypeIdentifier<wayward::Maybe<T>>*) {
    static const MaybeType<T>* p = new MaybeType<T>{get_type<T>()};
    return p;
  }
}

#endif // PERSISTENCE_TYPES_HPP_INCLUDED

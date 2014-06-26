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
#include <wayward/support/data_franca/spectator.hpp>

namespace persistence {
  struct NothingType : ISQLType {
    bool is_nullable() const final { return true; }
    std::string name() const final { return "NothingType"; }
    Result<void> deserialize_data(AnyRef, const wayward::data_franca::ScalarSpectator&) const final { return Success; }
    Result<void> serialize_data(AnyConstRef, wayward::data_franca::ScalarMutator& target) const final { target << wayward::Nothing; return Success; }
    const TypeInfo& type_info() const final { return wayward::GetTypeInfo<NothingType>::Value; }
    ast::Ptr<ast::SingleValue> make_literal(AnyConstRef) const final { return ast::Ptr<ast::SingleValue>{ new ast::SQLFragmentValue("NULL") }; }
  };

  const NothingType* build_type(const TypeIdentifier<wayward::NothingType>*);

  struct StringType : DataTypeFor<std::string> {
    bool is_nullable() const final { return false; }
    std::string name() const final { return "std::string"; }

    bool has_value(const std::string& value) const final {
      return true;
    }

    Result<void> deserialize_value(std::string& value, const wayward::data_franca::ScalarSpectator& input) const override {
      input >> value;
      return Success;
    }

    Result<void> serialize_value(const std::string& value, wayward::data_franca::ScalarMutator& target) const override {
      target << value;
      return Success;
    }

    ast::Ptr<ast::SingleValue> make_literal(AnyConstRef data) const final {
      if (!data.is_a<std::string>()) {
        throw wayward::Error("StringType::make_literal called with data that isn't an std::string.");
      }
      return ast::Ptr<ast::SingleValue> { new ast::StringLiteral{*data.get<const std::string&>()} };
    }
  };

  const StringType* build_type(const TypeIdentifier<std::string>*);

  template <typename T>
  struct NumericType : DataTypeFor<T> {
    NumericType(std::string name) : name_(std::move(name)) {}
    bool is_nullable() const final { return false; }
    std::string name() const final { return name_; }
    size_t bits() const { return sizeof(T)*8; }
    bool is_signed() const { return std::is_signed<T>::value; }
    bool is_float() const { return std::is_floating_point<T>::value; }

    bool has_value(const T& value) const final { return true; }

    Result<void> deserialize_value(T& value, const wayward::data_franca::ScalarSpectator& source) const override {
      if (is_float()) {
        wayward::data_franca::Real r;
        if (source >> r) {
          value = static_cast<T>(r);
          return Success;
        }
      } else {
        wayward::data_franca::Integer n;
        if (source >> n) {
          value = static_cast<T>(n);
          return Success;
        }
      }
      return wayward::make_error<wayward::Error>("Input not an integer.");
    }

    Result<void> serialize_value(const T& value, wayward::data_franca::ScalarMutator& target) const override {
      if (is_float()) {
        target << static_cast<wayward::data_franca::Real>(value);
      } else {
        target << static_cast<wayward::data_franca::Integer>(value);
      }
      return Success;
    }

    ast::Ptr<ast::SingleValue> make_literal(AnyConstRef data) const final {
      if (!data.is_a<T>()) {
        throw wayward::Error("NumericType::make_literal called with a value that doesn't correspond to the expected type.");
      }
      return ast::Ptr<ast::SingleValue> { new ast::NumericLiteral{ (double)*data.get<T>() }};
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
  struct MaybeType : DataTypeFor<wayward::Maybe<T>> {
    MaybeType(const IDataTypeFor<T>* inner_type) : inner_type_(inner_type) {}
    std::string name() const final { return detail::maybe_type_name(inner_type_); }
    bool is_nullable() const { return true; }

    bool has_value(const wayward::Maybe<T>& value) const final {
      return static_cast<bool>(value);
    }

    Result<void> deserialize_value(wayward::Maybe<T>& value, const wayward::data_franca::ScalarSpectator& source) const final {
      if (source) {
        T val;
        auto r = inner_type_->deserialize_value(val, source);
        value = std::move(val);
        return std::move(r);
      } else {
        value = wayward::Nothing;
        return Success;
      }

    }

    Result<void> serialize_value(const wayward::Maybe<T>& value, wayward::data_franca::ScalarMutator& target) const final {
      if (value) {
        return inner_type_->serialize_value(*value, target);
      } else {
        target << wayward::Nothing;
        return Success;
      }
    }

    ast::Ptr<ast::SingleValue> make_literal(AnyConstRef data) const final {
      if (!data.is_a<wayward::Maybe<T>>()) {
        throw wayward::Error("MaybeType::make_literal called with data that isn't a Maybe<T>.");
      }
      auto& m = *data.get<const Maybe<T>&>();
      if (m) {
        return inner_type_->make_literal(*m);
      }
      return ast::Ptr<ast::SingleValue> { new ast::SQLFragmentValue("NULL") };
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

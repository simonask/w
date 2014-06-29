#pragma once
#ifndef WAYWARD_SUPPORT_TYPES_HPP_INCLUDED
#define WAYWARD_SUPPORT_TYPES_HPP_INCLUDED

#include <wayward/support/type.hpp>
#include <wayward/support/data_visitor.hpp>
#include <wayward/support/maybe.hpp>

namespace wayward {
  struct NothingTypeType : IType {
    bool is_nullable() const final { return true; }
    std::string name() const final { return "NothingType"; }
    const TypeInfo& type_info() const { return GetTypeInfo<NothingType>::Value; }
    void visit_data(AnyRef data, DataVisitor& visitor) const { visitor.visit_nil(); }
    void visit_data(AnyConstRef data, DataVisitor& visitor) const { visitor.visit_nil(); }
  };
  const NothingTypeType* build_type(const TypeIdentifier<NothingType>*);

  struct StringType : DataTypeFor<std::string> {
    bool is_nullable() const final { return false; }
    std::string name() const final { return "std::string"; }
    bool has_value(const std::string& value) const final { return true; }
    void visit(std::string& value, DataVisitor& visitor) const final { visitor(value); }
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

    void visit(T& value, DataVisitor& visitor) const final {
      visitor(value);
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
  struct MaybeType : DataTypeFor<Maybe<T>> {
    MaybeType() {}
    std::string name() const final { return detail::maybe_type_name(get_type<T>()); }
    bool is_nullable() const { return true; }

    bool has_value(const wayward::Maybe<T>& value) const final {
      return static_cast<bool>(value);
    }

    void visit(Maybe<T>& value, DataVisitor& visitor) const final {
      if (visitor.can_modify()) {
        if (visitor.is_nil_at_current()) {
          value = Nothing;
        } else {
          value = T{};
          visitor(*value);
        }
      } else {
        if (value) {
          get_type<T>()->visit_data(*value, visitor);
        } else {
          visitor.visit_nil();
        }
      }
    }
  };

  template <typename T>
  const MaybeType<T>* build_type(const TypeIdentifier<Maybe<T>>*) {
    static const MaybeType<T>* p = new MaybeType<T>{};
    return p;
  }
}

#endif // WAYWARD_SUPPORT_TYPES_HPP_INCLUDED


#pragma once
#ifndef PERSISTENCE_TYPES_HPP_INCLUDED
#define PERSISTENCE_TYPES_HPP_INCLUDED

#include <string>
#include <cstdint>
#include <sstream>

#include <persistence/type.hpp>
#include <persistence/result_set.hpp>
#include <wayward/support/maybe.hpp>

namespace persistence {
  struct StringType : IDataTypeFor<std::string> {
    bool is_nullable() const final { return false; }
    std::string name() const final { return "std::string"; }

    bool has_value(const std::string& value) const final {
      return true;
    }

    void extract_from_results(std::string& value, const IResultSet& results, size_t row, const std::string& col) const final {
      value = results.get(row, col);
    }
  };

  template <>
  struct BuildType<std::string> {
    static const StringType* build();
  };

  template <typename T>
  struct NumericType : IDataTypeFor<T> {
    NumericType(std::string name) : name_(std::move(name)) {}
    bool is_nullable() const final { return false; }
    std::string name() const final { return name_; }
    size_t bits() const { return sizeof(T)*8; }
    bool is_signed() const { return std::is_signed<T>::value; }
    bool is_float() const { return std::is_floating_point<T>::value; }

    bool has_value(const T& value) const final { return true; }

    void extract_from_results(T& value, const IResultSet& results, size_t row, const std::string& col) const final {
      std::stringstream ss;
      ss.str(results.get(row, col));
      ss >> value; // TODO: Check failbits.
    }
  private:
    std::string name_;
  };

  template <>
  struct BuildType<std::int32_t> {
    static const NumericType<std::int32_t>* build();
  };

  template <>
  struct BuildType<std::int64_t> {
    static const NumericType<std::int64_t>* build();
  };

  template <>
  struct BuildType<std::uint32_t> {
    static const NumericType<std::uint32_t>* build();
  };

  template <>
  struct BuildType<std::uint64_t> {
    static const NumericType<std::uint64_t>* build();
  };

  template <>
  struct BuildType<float> {
    static const NumericType<float>* build();
  };

  template <>
  struct BuildType<double> {
    static const NumericType<double>* build();
  };

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

    void extract_from_results(wayward::Maybe<T>& value, const IResultSet& results, size_t row, const std::string& col) const final {
      if (results.is_null_at(row, col)) {
        value = wayward::Nothing;
      } else {
        T tmp;
        inner_type_->extract_from_results(tmp, results, row, col);
        value = std::move(tmp);
      }
    }
  private:
    const IDataTypeFor<T>* inner_type_ = nullptr;
  };

  template <typename T>
  struct BuildType<wayward::Maybe<T>> {
    static const MaybeType<T>* build() {
      static const MaybeType<T>* p = new MaybeType<T>{get_type<T>()};
      return p;
    }
  };
}

#endif // PERSISTENCE_TYPES_HPP_INCLUDED

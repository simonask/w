#pragma once
#ifndef PERSISTENCE_TYPES_HPP_INCLUDED
#define PERSISTENCE_TYPES_HPP_INCLUDED

#include <string>
#include <cstdint>

#include <persistence/type.hpp>
#include <wayward/support/maybe.hpp>

namespace persistence {
  struct StringType : IType {
    bool is_nullable() const final { return false; }
    std::string name() const final { return "std::string"; }
  };

  template <>
  struct BuildType<std::string> {
    static const StringType* build();
  };

  struct NumericType : IType {
    NumericType(std::string name, std::uint8_t bits, bool is_signed, bool is_float) : bits_(bits), is_signed_(is_signed), is_float_(is_float) {}
    bool is_nullable() const final { return false; }
    std::string name() const final { return name_; }
    size_t bits() const { return bits_; }
    bool is_signed() const { return is_signed_; }
    bool is_float() const { return is_float_; }
  private:
    std::string name_;
    std::uint8_t bits_;
    bool is_signed_ = true;
    bool is_float_ = true;
  };

  template <>
  struct BuildType<std::int32_t> {
    static const NumericType* build();
  };

  template <>
  struct BuildType<std::int64_t> {
    static const NumericType* build();
  };

  template <>
  struct BuildType<std::uint32_t> {
    static const NumericType* build();
  };

  template <>
  struct BuildType<std::uint64_t> {
    static const NumericType* build();
  };

  template <>
  struct BuildType<float> {
    static const NumericType* build();
  };

  template <>
  struct BuildType<double> {
    static const NumericType* build();
  };

  struct MaybeType : IType {
    MaybeType(const IType* inner_type) : inner_type_(inner_type) {}
    std::string name() const final;
    bool is_nullable() const { return true; }
  private:
    const IType* inner_type_ = nullptr;
  };

  template <typename T>
  struct BuildType<w::Maybe<T>> {
    static const MaybeType* build() {
      static const MaybeType* p = new MaybeType{get_type<T>()};
      return p;
    }
  };
}

#endif // PERSISTENCE_TYPES_HPP_INCLUDED

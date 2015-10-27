#pragma once
#ifndef WAYWARD_SUPPORT_BITFLAGS_HPP_INCLUDED
#define WAYWARD_SUPPORT_BITFLAGS_HPP_INCLUDED

#include <type_traits>

namespace wayward {
  template <typename Enum>
  struct Bitflags {
    Bitflags() : value_(0) {}
    Bitflags(Enum value) : value_(static_cast<Raw>(value)) {}
    Bitflags(const Bitflags<Enum>&) = default;
    Bitflags<Enum>& operator=(const Bitflags<Enum>&) = default;

    bool operator==(const Bitflags<Enum>& other) const { return value_ == other.value_; }
    bool operator!=(const Bitflags<Enum>& other) const { return value_ != other.value_; }

    Bitflags<Enum> operator|(Bitflags<Enum> other) const { return value_ | other.value_; }
    Bitflags<Enum> operator&(Bitflags<Enum> other) const { return value_ & other.value_; }
    Bitflags<Enum> operator^(Bitflags<Enum> other) const { return value_ ^ other.value_; }
    Bitflags<Enum> operator~() const { return ~value_; }

    Bitflags<Enum>& operator|=(Bitflags<Enum> other) { value_ |= other.value_; return *this; }
    Bitflags<Enum>& operator&=(Bitflags<Enum> other) { value_ &= other.value_; return *this; }
    Bitflags<Enum>& operator^=(Bitflags<Enum> other) { value_ ^= other.value_; return *this; }

    operator bool() const { return (bool)value_; }
  private:
    using Raw = typename std::underlying_type<Enum>::type;
    Bitflags(Raw value) : value_(value) {}
    Raw value_;
  };
}

#endif // WAYWARD_SUPPORT_BITFLAGS_HPP_INCLUDED

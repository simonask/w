#pragma once
#ifndef PERSISTENCE_DATA_REF_HPP_INCLUDED
#define PERSISTENCE_DATA_REF_HPP_INCLUDED

#include <persistence/type.hpp>

#include <wayward/support/maybe.hpp>
#include <wayward/support/data_franca/adapter.hpp>

namespace persistence {
  using wayward::Maybe;
  using wayward::Nothing;

  struct DataRef {
    DataRef(const DataRef&) = default;
    DataRef& operator=(const DataRef&) = default;

    template <typename T>
    DataRef(const T& ref)
    : type_(get_type<T>())
    , memory_(reinterpret_cast<const void*>(&ref))
    , make_reader_([](const void* memory) { return wayward::data_franca::make_reader(*reinterpret_cast<const T*>(memory)); }) {}

    const IType* type() const { return type_; }

    template <typename T>
    bool is_a() const { return type_ == get_type<T>(); }

    template <typename T, typename F>
    auto fmap(F&& f) -> Maybe<decltype(f(std::declval<T>()))> {
      if (is_a<T>()) {
        return f(*get<T>());
      }
      return Nothing;
    }

    template <typename T, typename F>
    const DataRef& when(F&& f) const {
      if (is_a<T>()) {
        f(*get<T>());
      }
      return *this;
    }

    wayward::data_franca::ReaderPtr
    reader() const {
      return make_reader_(memory_);
    }
  private:
    using ReaderPtrFunc = wayward::data_franca::ReaderPtr(*)(const void*);

    const IType* type_;
    const void* memory_;
    ReaderPtrFunc make_reader_;

    template <typename T>
    const T* get() const { return reinterpret_cast<const T*>(memory_); }
  };
}

#endif // PERSISTENCE_DATA_REF_HPP_INCLUDED

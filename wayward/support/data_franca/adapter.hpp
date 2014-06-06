#pragma once
#ifndef WAYWARD_SUPPORT_DATA_FRANCA_ADAPTER_HPP_INCLUDED
#define WAYWARD_SUPPORT_DATA_FRANCA_ADAPTER_HPP_INCLUDED

#include <wayward/support/data_franca/reader.hpp>
#include <wayward/support/data_franca/writer.hpp>

#include <wayward/support/meta.hpp>

namespace wayward {
  namespace data_franca {
    struct IAdapter : IReader, IWriter {
      virtual ~IAdapter() {}
    };

    template <typename T>
    struct AdapterBase : IAdapter {
      AdapterBase(T& ref) : ref_(ref) {}

      // IReader interface:
      DataType type() const override { return DataType::Nothing; }
      Maybe<Boolean> get_boolean() const override { return Nothing; }
      Maybe<Integer> get_integer() const override { return Nothing; }
      Maybe<Real>    get_real()    const override { return Nothing; }
      Maybe<String>  get_string()  const override { return Nothing; }
      bool has_key(const String& key) const override { return false; }
      ReaderPtr get(const String& key) const override { return nullptr; }
      size_t   length()       const override { return 0; }
      ReaderPtr at(size_t idx) const override { return nullptr; }
      ReaderEnumeratorPtr enumerator() const override { return nullptr; }

      // IWriter interface:
      bool set_boolean(Boolean b) override { return false; }
      bool set_integer(Integer n) override { return false; }
      bool set_real(Real r) override       { return false; }
      bool set_string(String str) override { return false; }
      AdapterPtr reference_at_index(size_t idx) override { return nullptr; }
      AdapterPtr push_back() override { return nullptr; }
      AdapterPtr reference_at_key(const String& key) override { return nullptr; }
      bool erase(const String& key) override { return false; }

      T& ref_;
    };

    template <typename T, typename Enable = void> struct Adapter;
    template <typename T, typename Enable = void> struct OwningAdapter;


    template <typename T> struct GetAdapter;

    template <typename T>
    auto make_adapter(T&& object) ->
    // C++14 now please...
    decltype(
      GetAdapter<typename meta::RemoveConstRef<T>::Type>::get(std::forward<T>(object))
    ) {
      return GetAdapter<typename meta::RemoveConstRef<T>::Type>::get(std::forward<T>(object));
    }

    template <typename T>
    ReaderPtr make_reader(const T& object) {
      return GetAdapter<T>::get(object);
    }

    template <typename T>
    struct GetAdapter {
      static AdapterPtr get(T& object) {
        return std::static_pointer_cast<IAdapter>(std::make_shared<Adapter<T>>(object));
      }
      static ReaderPtr get(const T& object) {
        // It's OK to const_cast here, because we immediately upcast to IReader, which is a const-only interface.
        return std::static_pointer_cast<const IReader>(std::make_shared<Adapter<T>>(const_cast<T&>(object)));
      }
    };
  }
}

#endif // WAYWARD_SUPPORT_DATA_FRANCA_ADAPTER_HPP_INCLUDED

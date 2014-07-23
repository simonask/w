#pragma once
#ifndef WAYWARD_SUPPORT_DATA_FRANCA_ADAPTER_HPP_INCLUDED
#define WAYWARD_SUPPORT_DATA_FRANCA_ADAPTER_HPP_INCLUDED

#include <wayward/support/data_franca/reader.hpp>
#include <wayward/support/data_franca/writer.hpp>

#include <wayward/support/meta.hpp>
#include <wayward/support/bitflags.hpp>

namespace wayward {
  namespace data_franca {
    enum class Options {
      None = 0,

      // A flag that indicates that readers/adapters are allowed to make potentially slow
      // requests to fetch all requested data.
      AllowLoad = 1,
    };

    struct IAdapter : IReader, IWriter {
      virtual ~IAdapter() {}
    };

    template <typename T>
    struct AdapterBase : IAdapter {
      AdapterBase(T& ref, Bitflags<Options> opts) : ref_(ref), options_(opts) {}

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
      bool set_nothing() override { return false; }
      bool set_boolean(Boolean b) override { return false; }
      bool set_integer(Integer n) override { return false; }
      bool set_real(Real r) override       { return false; }
      bool set_string(String str) override { return false; }
      AdapterPtr reference_at_index(size_t idx) override { return nullptr; }
      AdapterPtr push_back() override { return nullptr; }
      AdapterPtr reference_at_key(const String& key) override { return nullptr; }
      bool erase(const String& key) override { return false; }

      T& ref_;
      Bitflags<Options> options_;
    };

    template <typename T, typename Enable = void> struct Adapter;

    template <typename T> struct GetAdapter;

    template <typename T>
    auto make_adapter(T&& object, Bitflags<Options> options) {
      return GetAdapter<typename meta::RemoveConstRef<T>::Type>::get(std::forward<T>(object), options);
    }

    template <typename T>
    ReaderPtr make_reader(T&& object, Bitflags<Options> options = Options::None) {
      return GetAdapter<typename meta::RemoveConstRef<T>::Type>::get(std::forward<T>(object), options);
    }

    template <typename T>
    struct GetAdapter {
      static AdapterPtr get(T& object, Bitflags<Options> options) {
        return std::static_pointer_cast<IAdapter>(std::make_shared<Adapter<T>>(object, options));
      }
      static ReaderPtr get(const T& object, Bitflags<Options> options) {
        // It's OK to const_cast here, because we immediately upcast to IReader, which is a const-only interface.
        return std::static_pointer_cast<const IReader>(std::make_shared<Adapter<T>>(const_cast<T&>(object), options));
      }
    };

    template <typename T>
    struct OwningAdapter : Adapter<T> {
      T owned_;
      OwningAdapter(T object, Bitflags<Options> options) : Adapter<T>(owned_, options), owned_(std::move(object)) {}
    };

    template <typename T>
    AdapterPtr make_owning_adapter(T&& object, Bitflags<Options> options = Options::None) {
      return AdapterPtr{ new OwningAdapter<typename meta::RemoveConstRef<T>::Type>{ std::forward<T>(object), options } };
    }

    template <typename T>
    ReaderPtr make_owning_reader(T&& object, Bitflags<Options> options = Options::None) {
      return ReaderPtr{ new OwningAdapter<typename meta::RemoveConstRef<T>::Type>{ std::forward<T>(object), options } };
    }
  }
}

#endif // WAYWARD_SUPPORT_DATA_FRANCA_ADAPTER_HPP_INCLUDED

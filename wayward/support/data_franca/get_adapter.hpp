#pragma once
#ifndef WAYWARD_SUPPORT_DATA_FRANCA_GET_ADAPTER_HPP_INCLUDED
#define WAYWARD_SUPPORT_DATA_FRANCA_GET_ADAPTER_HPP_INCLUDED

namespace wayward {
  namespace data_franca {
    template <typename T> struct GetAdapter;

    struct IReader;
    struct IWriter;
    using ReaderPtr = std::shared_ptr<const IReader>;
    using AdapterPtr = std::shared_ptr<IWriter>;

    template <typename T>
    auto make_adapter(T&& object) ->
    // C++14 now please...
    decltype(
      GetAdapter<typename meta::RemoveConstRef<T>::Type>::get(std::forward<T>(object))
    ) {
      return GetAdapter<typename meta::RemoveConstRef<T>::Type>::get(std::forward<T>(object));
    }

    template <typename T>
    ReaderPtr make_reader(T&& object) {
      return std::static_pointer_cast<const IReader>(make_adapter(std::forward<T>(object)));
    }

    template <typename T>
    struct GetAdapter {
      AdapterPtr get(T& object) {
        return std::static_pointer_cast<IWriter>(std::make_shared<StandardAdapter<T>>(object));
      }
      ReaderPtr get(const T& object) {
        return std::static_pointer_cast<const IReader>(std::make_shared<StandardReader<T>>(object));
      }
    };
  }
}

#endif // WAYWARD_SUPPORT_DATA_FRANCA_GET_ADAPTER_HPP_INCLUDED

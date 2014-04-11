#pragma once
#ifndef PERSISTENCE_ADAPTER_HPP_INCLUDED
#define PERSISTENCE_ADAPTER_HPP_INCLUDED

#include <persistence/connection.hpp>

namespace persistence {
  struct IAdapter {
    virtual ~IAdapter() {}
    virtual std::unique_ptr<IConnection> connect(std::string connection_url) const = 0;
  };

  const IAdapter* adapter_for_protocol(const std::string& protocol);

  namespace detail {
    struct AdapterLink {
      const char* name = nullptr;
      const IAdapter* adapter = nullptr;
      AdapterLink* next = nullptr;
      AdapterLink(const char* name, const IAdapter* adapter) : name(name), adapter(adapter) {}
    };

    void register_adapter(AdapterLink* adapter);
    void unregister_adapter(AdapterLink* adapter);
  }

  /*
    This struct must be statically allocated!
  */
  template <typename T>
  struct AdapterRegistrar {
    AdapterRegistrar(const char* name) : link_{name, &adapter_} {
      persistence::detail::register_adapter(&link_);
    }
    ~AdapterRegistrar() {
      persistence::detail::unregister_adapter(&link_);
    }
    detail::AdapterLink link_;
    T adapter_;
  };
}

#endif // PERSISTENCE_ADAPTER_HPP_INCLUDED

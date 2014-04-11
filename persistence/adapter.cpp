#include <persistence/adapter.hpp>

namespace persistence {
  namespace detail {
    static AdapterLink* g_adapter_link_head = nullptr;

    void register_adapter(AdapterLink* link) {
      link->next = g_adapter_link_head;
      g_adapter_link_head = link;
    }

    void unregister_adapter(AdapterLink* link) {
      for (AdapterLink** p = &g_adapter_link_head; *p; p = &(*p)->next) {
        if (*p == link) {
          *p = link->next;
          return;
        }
      }
    }
  }

  const IAdapter* adapter_for_protocol(const std::string& name) {
    for (auto p = detail::g_adapter_link_head; p; p = p->next) {
      if (name == p->name) {
        return p->adapter;
      }
    }
    return nullptr;
  }
}

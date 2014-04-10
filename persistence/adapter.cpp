#include <persistence/adapter.hpp>

namespace persistence {
  namespace detail {
    static AdapterLink* g_adapter_link_head = nullptr;
    static AdapterLink* g_adapter_link_tail = nullptr;

    void register_adapter(AdapterLink* link) {
      if (g_adapter_link_head == nullptr)
        g_adapter_link_head = link;
      if (g_adapter_link_tail != nullptr)
        g_adapter_link_tail->next = link;
      g_adapter_link_tail = link;
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

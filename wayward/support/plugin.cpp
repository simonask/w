#include <wayward/support/plugin.hpp>

#include <dlfcn.h>

namespace wayward {
  void load_plugin(std::string name) {
    void* handle = ::dlopen(name.c_str(), RTLD_LOCAL);
    if (handle == nullptr) {
      throw PluginError(wayward::format("Plugin '{0}' could not be loaded.", name));
    }
    void* init = ::dlsym(handle, "wayward_init_plugin");
    if (init == nullptr) {
      throw PluginError(wayward::format("Plugin '{0}' does not contain a function named 'wayward_init_plugin'.", name));
    }
    auto init_func = (void(*)())init;
    init_func();
  }
}

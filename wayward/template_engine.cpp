#include <wayward/template_engine.hpp>

namespace wayward {
  namespace {
    std::map<std::string, TemplateEngineInstantiator> g_template_engines;
    std::shared_ptr<ITemplateEngine> g_current_engine;
  }

  void register_template_engine(std::string name, TemplateEngineInstantiator func) {
    g_template_engines[name] = func;
  }

  std::shared_ptr<ITemplateEngine> template_engine(const std::string& name, const Options& options) {
    auto it = g_template_engines.find(name);
    if (it != g_template_engines.end()) {
      auto template_engine = it->second();
      template_engine->initialize(std::move(options));
      return std::move(template_engine);
    } else {
      throw TemplateError{wayward::format("Unknown template engine: {0}", name)};
    }
  }

  void set_template_engine(const std::string& name, Options options) {
    g_current_engine = template_engine(name, std::move(options));
  }

  std::shared_ptr<ITemplateEngine> current_template_engine() {
    if (!g_current_engine) {
      throw TemplateError{"No template engine has been configured."};
    }
    return g_current_engine;
  }
}

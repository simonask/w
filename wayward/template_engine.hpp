#pragma once
#ifndef WAYWARD_TEMPLATE_ENGINE_HPP_INCLUDED
#define WAYWARD_TEMPLATE_ENGINE_HPP_INCLUDED

#include <wayward/support/options.hpp>
#include <wayward/support/error.hpp>

namespace wayward {
  struct TemplateError : Error {
    TemplateError(const std::string& msg) : Error(msg) {}
  };

  struct ITemplateEngine {
    virtual ~ITemplateEngine() {}
    /*
      Initialize a template engine with options.
    */
    virtual void initialize(Options options) = 0;

    /*
      Render a template.
    */
    virtual std::string render(const std::string& template_name, Options params) = 0;
  };

  using TemplateEngineInstantiator = std::shared_ptr<ITemplateEngine>(*)();

  void register_template_engine(std::string name, TemplateEngineInstantiator instantiator_function);
  std::shared_ptr<ITemplateEngine> template_engine(const std::string& name, const Options& options = Options{});
  void set_template_engine(const std::string& name, Options options = Options{});

  std::shared_ptr<ITemplateEngine> current_template_engine();
}

#endif // WAYWARD_TEMPLATE_ENGINE_HPP_INCLUDED

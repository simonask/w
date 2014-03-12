#pragma once
#ifndef PERSISTENCE_BUILDER_HPP_INCLUDED
#define PERSISTENCE_BUILDER_HPP_INCLUDED

#include <persistence/ast.hpp>

namespace persistence {
  struct Relation {
    std::string name;
  };

  struct ProjectionBuilder {
    ProjectionBuilder() {}
  private:
    ast::SelectQuery query;
  };
}

#endif // PERSISTENCE_BUILDER_HPP_INCLUDED

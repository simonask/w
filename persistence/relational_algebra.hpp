#pragma once
#ifndef PERSISTENCE_RELATIONAL_ALGEBRA_HPP_INCLUDED
#define PERSISTENCE_RELATIONAL_ALGEBRA_HPP_INCLUDED

#include <persistence/ast.hpp>

namespace persistence {
  namespace relational_algebra {
    struct Condition {
      std::unique_ptr<ast::Condition> cond;
    };

    struct Aggregate {

    };

    struct LiteralSQL {
      std::string sql;
    };

    struct Column {
      std::unique_ptr<ast::ColumnReference> col;

      Condition like(std::string cmp);
      Condition operator<(Column other);
      Condition operator<(Aggregate agg);
      Condition operator<(int64_t numeric_literal);
      Condition operator<(std::string string_literal);
    };

    struct Projection {
      explicit Projection(std::string);

      Projection where(Condition);
    };

    Projection projection(std::string relation) {
      return Projection{relation};
    }
  }
}

#endif // PERSISTENCE_RELATIONAL_ALGEBRA_HPP_INCLUDED

#pragma once
#ifndef PERSISTENCE_POSTGRESQL_RENDERERS_HPP_INCLUDED
#define PERSISTENCE_POSTGRESQL_RENDERERS_HPP_INCLUDED

#include <persistence/ast.hpp>
#include <persistence/postgresql.hpp>

namespace persistence {
  using namespace persistence::ast;

  struct PostgreSQLQueryRenderer : ast::ISQLQueryRenderer {
    PostgreSQLConnection& conn;
    PostgreSQLQueryRenderer(PostgreSQLConnection& conn) : conn(conn) {}

    std::string render(const SelectQuery& x) final;
    std::string render(const UpdateQuery& x) final;
    std::string render(const DeleteQuery& x) final;
    std::string render(const InsertQuery& x) final;
  };

  struct PostgreSQLValueRenderer : ast::ISQLValueRenderer {
    PostgreSQLConnection& conn;
    PostgreSQLValueRenderer(PostgreSQLConnection& conn) : conn(conn) {}

    std::string render(const StarFrom& x) final;
    std::string render(const StringLiteral& x) final;
    std::string render(const NumericLiteral& x) final;
    std::string render(const BooleanLiteral& x) final;
    std::string render(const ColumnReference& x) final;
    std::string render(const Aggregate& x) final;
    std::string render(const List& x) final;
    std::string render(const CaseSimple& x) final;
    std::string render(const Case& x) final;
    std::string render(const NotCondition& x) final;
    std::string render(const UnaryCondition& x) final;
    std::string render(const BinaryCondition& x) final;
    std::string render(const BetweenCondition& x) final;
    std::string render(const LogicalCondition& x) final;
    std::string render(const SelectQuery& x) final;
  };
}

#endif // PERSISTENCE_POSTGRESQL_RENDERERS_HPP_INCLUDED

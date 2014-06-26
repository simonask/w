#pragma once
#ifndef PERSISTENCE_ADAPTERS_POSTGRESQL_RENDERERS_HPP_INCLUDED
#define PERSISTENCE_ADAPTERS_POSTGRESQL_RENDERERS_HPP_INCLUDED

#include <persistence/ast.hpp>
#include <persistence/connection.hpp>
#include <persistence/relational_algebra.hpp>

namespace persistence {
  using namespace persistence::ast;
  using relational_algebra::IResolveSymbolicRelation;

  struct PostgreSQLQueryRenderer : ast::ISQLQueryRenderer {
    IConnection& conn;
    const IResolveSymbolicRelation& symbolic_relation_resolver;
    PostgreSQLQueryRenderer(IConnection& conn, const IResolveSymbolicRelation& rel) : conn(conn), symbolic_relation_resolver(rel) {}

    std::string render(const ast::SelectQuery& x) final;
    std::string render(const ast::UpdateQuery& x) final;
    std::string render(const ast::DeleteQuery& x) final;
    std::string render(const ast::InsertQuery& x) final;
  };

  struct PostgreSQLValueRenderer : ast::ISQLValueRenderer {
    IConnection& conn;
    const IResolveSymbolicRelation& symbolic_relation_resolver;
    PostgreSQLValueRenderer(IConnection& conn, const IResolveSymbolicRelation& rel) : conn(conn), symbolic_relation_resolver(rel) {}

    std::string render(const ast::StarFrom& x) final;
    std::string render(const ast::StringLiteral& x) final;
    std::string render(const ast::NumericLiteral& x) final;
    std::string render(const ast::BooleanLiteral& x) final;
    std::string render(const ast::ColumnReference& x) final;
    std::string render(const ast::ColumnReferenceWithSymbolicRelation& x) final;
    std::string render(const ast::Aggregate& x) final;
    std::string render(const ast::List& x) final;
    std::string render(const ast::CaseSimple& x) final;
    std::string render(const ast::Case& x) final;
    std::string render(const ast::NotCondition& x) final;
    std::string render(const ast::UnaryCondition& x) final;
    std::string render(const ast::BinaryCondition& x) final;
    std::string render(const ast::BetweenCondition& x) final;
    std::string render(const ast::LogicalCondition& x) final;
    std::string render(const ast::SelectQuery& x) final;
  };
}

#endif // PERSISTENCE_ADAPTERS_POSTGRESQL_RENDERERS_HPP_INCLUDED

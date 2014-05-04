#pragma once
#ifndef PERSISTENCE_AST_HPP_INCLUDED
#define PERSISTENCE_AST_HPP_INCLUDED

#include <vector>
#include <memory>
#include <string>
#include <wayward/support/cloning_ptr.hpp>
#include <wayward/support/maybe.hpp>

namespace persistence {
  namespace ast {
    using wayward::CloningPtr;
    using wayward::Cloneable;
    using wayward::ICloneable;
    using wayward::Maybe;

    struct StarFrom;
    struct StringLiteral;
    struct NumericLiteral;
    struct BooleanLiteral;
    struct ColumnReference;
    struct ColumnReferenceWithSymbolicRelation;
    struct Aggregate;
    struct List;
    struct CaseSimple;
    struct Case;
    struct NotCondition;
    struct UnaryCondition;
    struct BinaryCondition;
    struct BetweenCondition;
    struct LogicalCondition;
    struct SelectQuery;
    struct UpdateQuery;
    struct DeleteQuery;
    struct InsertQuery;

    struct ISQLValueRenderer {
      virtual ~ISQLValueRenderer() {}
      virtual std::string render(const StarFrom& x) = 0;
      virtual std::string render(const StringLiteral& x) = 0;
      virtual std::string render(const NumericLiteral& x) = 0;
      virtual std::string render(const BooleanLiteral& x) = 0;
      virtual std::string render(const ColumnReference& x) = 0;
      virtual std::string render(const ColumnReferenceWithSymbolicRelation& x) = 0;
      virtual std::string render(const Aggregate& x) = 0;
      virtual std::string render(const List& x) = 0;
      virtual std::string render(const CaseSimple& x) = 0;
      virtual std::string render(const Case& x) = 0;
      virtual std::string render(const NotCondition& x) = 0;
      virtual std::string render(const UnaryCondition& x) = 0;
      virtual std::string render(const BinaryCondition& x) = 0;
      virtual std::string render(const BetweenCondition& x) = 0;
      virtual std::string render(const LogicalCondition& x) = 0;
      virtual std::string render(const SelectQuery& x) = 0;
    };

    struct ISQLQueryRenderer {
      virtual ~ISQLQueryRenderer() {}
      virtual std::string render(const SelectQuery& x) = 0;
      virtual std::string render(const UpdateQuery& x) = 0;
      virtual std::string render(const DeleteQuery& x) = 0;
      virtual std::string render(const InsertQuery& x) = 0;
    };

    struct Value : ICloneable {
      virtual ~Value() {}
      virtual std::string to_sql(ISQLValueRenderer& visitor) const = 0;
    };

    // "relation".*
    struct StarFrom : Value {
      std::string relation;
      virtual std::string to_sql(ISQLValueRenderer& visitor) const = 0;
    };

    struct SingleValue : Value {
      virtual ~SingleValue() {}
    };

    struct Literal : SingleValue {
      virtual ~Literal() {}
    };

    struct SQLFragmentValue : Cloneable<SQLFragmentValue, SingleValue> {
      std::string sql;
      std::string to_sql(ISQLValueRenderer& visitor) const final { return sql; }

      SQLFragmentValue(const SQLFragmentValue&) = default;
      SQLFragmentValue(std::string sql) : sql(std::move(sql)) {}
    };

    struct StringLiteral : Cloneable<StringLiteral, SingleValue> {
      StringLiteral(std::string literal) : literal(std::move(literal)) {}
      std::string literal; // unsanitized!
      std::string to_sql(ISQLValueRenderer& visitor) const final { return visitor.render(*this); }
    };

    struct NumericLiteral : Cloneable<NumericLiteral, SingleValue> {
      double literal;
      NumericLiteral(double literal) : literal(literal) {}
      std::string to_sql(ISQLValueRenderer& visitor) const final { return visitor.render(*this); }
    };

    struct BooleanLiteral : Cloneable<BooleanLiteral, SingleValue> {
      bool value;
      std::string to_sql(ISQLValueRenderer& visitor) const final { return visitor.render(*this); }
    };

    // "relation"."column"
    struct ColumnReference : Cloneable<ColumnReference, SingleValue> {
      virtual ~ColumnReference() {}
      ColumnReference(std::string relation, std::string column) : relation(std::move(relation)), column(std::move(column)) {}

      std::string relation;
      std::string column;

      std::string to_sql(ISQLValueRenderer& visitor) const final { return visitor.render(*this); }
    };

    using SymbolicRelation = std::uintptr_t;

    struct ColumnReferenceWithSymbolicRelation : Cloneable<ColumnReferenceWithSymbolicRelation, SingleValue> {
      virtual ~ColumnReferenceWithSymbolicRelation() {}
      ColumnReferenceWithSymbolicRelation(SymbolicRelation relation, std::string column) : relation(relation), column(std::move(column)) {}

      SymbolicRelation relation;
      std::string column;

      std::string to_sql(ISQLValueRenderer& visitor) const final { return visitor.render(*this); }
    };

    // function(arguments...)
    struct Aggregate : Cloneable<Aggregate, SingleValue> {
      virtual ~Aggregate() {}
      Aggregate(std::string function, std::vector<CloningPtr<SingleValue>> arguments) : function(std::move(function)), arguments(std::move(arguments)) {}

      std::string function;
      std::vector<CloningPtr<SingleValue>> arguments;

      std::string to_sql(ISQLValueRenderer& visitor) const final { return visitor.render(*this); }
    };

    // (elements...)
    struct List : Cloneable<List, SingleValue> {
      virtual ~List() {}
      std::vector<CloningPtr<SingleValue>> elements;

      std::string to_sql(ISQLValueRenderer& visitor) const final { return visitor.render(*this); }
    };

    // CASE value
    // WHEN a THEN a_v
    // WHEN b THEN b_v
    // ELSE otherwise
    // END
    struct CaseSimple : Cloneable<CaseSimple, SingleValue> {
      CloningPtr<SingleValue> value;
      struct When {
        CloningPtr<SingleValue> when;
        CloningPtr<SingleValue> then;
      };
      CloningPtr<SingleValue> otherwise;

      std::string to_sql(ISQLValueRenderer& visitor) const final { return visitor.render(*this); }
    };

    // Conditions are values because they can be SELECT'ed as well.
    struct Condition : SingleValue {
      virtual ~Condition() {}
    };

    // CASE
    // WHEN cond1 THEN value1
    // WHEN cond2 THEN value2
    // ELSE otherwise
    // END
    struct Case : Cloneable<Case, SingleValue> {
      struct When {
        CloningPtr<Condition> cond;
        CloningPtr<SingleValue> then;
      };
      CloningPtr<SingleValue> otherwise;

      std::string to_sql(ISQLValueRenderer& visitor) const final { return visitor.render(*this); }
    };

    // fragment
    struct SQLFragmentCondition : Cloneable<SQLFragmentCondition, Condition> {
      std::string sql;

      std::string to_sql(ISQLValueRenderer& visitor) const final { return sql; }
    };

    // NOT (subcondition)
    struct NotCondition : Cloneable<NotCondition, Condition> {
      virtual ~NotCondition() {}

      CloningPtr<Condition> subcondition;

      std::string to_sql(ISQLValueRenderer& visitor) const final { return visitor.render(*this); }
    };

    // value op
    struct UnaryCondition : Cloneable<UnaryCondition, Condition> {
      virtual ~UnaryCondition() {}
      enum Cond {
        IsNull,
        IsNotNull,
        IsTrue,
        IsNotTrue,
        IsFalse,
        IsNotFalse,
        IsUnknown,
        IsNotUnknown,
      };
      CloningPtr<SingleValue> value;
      Cond op;

      std::string to_sql(ISQLValueRenderer& visitor) const final { return visitor.render(*this); }
    };

    // lhs op rhs
    struct BinaryCondition : Cloneable<BinaryCondition, Condition> {
      enum Cond {
        Eq,
        NotEq,
        LessThan,
        GreaterThan,
        LessThanOrEq,
        GreaterThanOrEq,
        In,
        NotIn,
        IsDistinctFrom,
        IsNotDistinctFrom,
        Like,
        ILike,
      };

      virtual ~BinaryCondition() {}
      BinaryCondition(CloningPtr<SingleValue> lhs, CloningPtr<SingleValue> rhs, Cond op) : lhs(std::move(lhs)), rhs(std::move(rhs)), op(op) {}

      CloningPtr<SingleValue> lhs;
      CloningPtr<SingleValue> rhs;
      Cond op;

      std::string to_sql(ISQLValueRenderer& visitor) const final { return visitor.render(*this); }
    };

    // value BETWEEN lower_bound AND upper_bound
    struct BetweenCondition : Cloneable<BetweenCondition, Condition> {
      virtual ~BetweenCondition() {}
      CloningPtr<SingleValue> value;
      CloningPtr<SingleValue> lower_bound;
      CloningPtr<SingleValue> upper_bound;

      std::string to_sql(ISQLValueRenderer& visitor) const final { return visitor.render(*this); }
    };

    // (lhs) op (rhs)
    struct LogicalCondition : Cloneable<LogicalCondition, Condition> {
      enum Cond {
        AND,
        OR,
      };

      virtual ~LogicalCondition() {}
      LogicalCondition(CloningPtr<Condition> lhs, CloningPtr<Condition> rhs, Cond op) : lhs(std::move(lhs)), rhs(std::move(rhs)), op(op) {}

      CloningPtr<Condition> lhs;
      CloningPtr<Condition> rhs;
      Cond op;

      std::string to_sql(ISQLValueRenderer& visitor) const final { return visitor.render(*this); }
    };

    struct IQuery {
      virtual ~IQuery() {}
      virtual std::string to_sql(ISQLQueryRenderer& visitor) const = 0;
    };

    struct Join : Cloneable<Join, ICloneable> {
      enum Type {
        Cross,
        Inner,
        LeftOuter,
        RightOuter,
        FullOuter,
      };

      Type type;
      std::string relation;
      std::string alias;
      CloningPtr<Condition> on;

      Join(Type type, std::string relation, std::string alias, CloningPtr<Condition> on) : type(type), relation(std::move(relation)), alias(std::move(alias)), on(std::move(on)) {}
    };

    struct SelectAlias {
      CloningPtr<ast::Value> value;
      wayward::Maybe<std::string> alias;

      SelectAlias() {}
      SelectAlias(CloningPtr<ast::Value> val) : value(std::move(val)) {}
      SelectAlias(CloningPtr<ast::Value> val, std::string alias) : value(std::move(val)), alias(std::move(alias)) {}
      SelectAlias(const SelectAlias&) = default;
      SelectAlias(SelectAlias&&) = default;
    };

    struct Ordering {
      CloningPtr<ast::Value> value;
      enum OrderingType {
        Ascending,
        Descending,
      };
      OrderingType ordering = Ascending;

      Ordering() {}
      Ordering(CloningPtr<ast::Value> val) : value(std::move(val)) {}
      Ordering(CloningPtr<ast::Value> val, OrderingType ordering) : value(std::move(val)), ordering(ordering) {}
      Ordering(const Ordering&) = default;
      Ordering(Ordering&&) = default;
    };

    // SELECT select FROM relation joins WHERE where GROUP BY group ORDER BY order order_descending
    // This is a SingleValue to provide support for subselects.
    struct SelectQuery : Cloneable<SelectQuery, SingleValue>, IQuery {
      virtual ~SelectQuery() {}
      std::vector<SelectAlias> select;
      std::string relation;
      Maybe<std::string> relation_alias;
      CloningPtr<ast::Condition> where;
      std::vector<CloningPtr<Join>> joins;
      std::vector<CloningPtr<ast::Value>> group;
      std::vector<Ordering> order;

      // TODO: Use Maybe
      ssize_t limit = -1;
      ssize_t offset = -1;

      std::string to_sql(ISQLQueryRenderer& visitor) const final { return visitor.render(*this); }
      std::string to_sql(ISQLValueRenderer& visitor) const final { return visitor.render(*this); }
    };

    struct UpdateQuery : Cloneable<UpdateQuery>, IQuery {
      virtual ~UpdateQuery() {}
      std::string relation;
      CloningPtr<ast::Condition> where;
      CloningPtr<size_t> limit;
      std::vector<std::string> columns;
      std::vector<CloningPtr<SingleValue>> values;

      std::string to_sql(ISQLQueryRenderer& visitor) const final { return visitor.render(*this); }
    };

    struct DeleteQuery : Cloneable<DeleteQuery>, IQuery {
      virtual ~DeleteQuery() {}
      std::string relation;
      CloningPtr<ast::Condition> where;
      CloningPtr<size_t> limit;

      std::string to_sql(ISQLQueryRenderer& visitor) const final { return visitor.render(*this); }
    };

    struct InsertQuery : Cloneable<InsertQuery>, IQuery {
      virtual ~InsertQuery() {}
      std::string relation;
      std::vector<std::string> columns;
      std::vector<CloningPtr<SingleValue>> values;

      std::string to_sql(ISQLQueryRenderer& visitor) const final { return visitor.render(*this); }
    };
  }
}

#endif // PERSISTENCE_AST_HPP_INCLUDED

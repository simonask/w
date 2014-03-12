#pragma once
#ifndef PERSISTENCE_AST_HPP_INCLUDED
#define PERSISTENCE_AST_HPP_INCLUDED

#include <vector>
#include <memory>
#include <string>

namespace persistence {
  namespace ast {
    struct StarFrom;
    struct StringLiteral;
    struct NumericLiteral;
    struct BooleanLiteral;
    struct ColumnReference;
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

    struct Value {
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

    struct StringLiteral : SingleValue {
      std::string literal; // unsanitized!
      std::string to_sql(ISQLValueRenderer& visitor) const final { return visitor.render(*this); }
    };

    struct NumericLiteral : SingleValue {
      std::string literal;
      std::string to_sql(ISQLValueRenderer& visitor) const final { return visitor.render(*this); }
    };

    struct BooleanLiteral : SingleValue {
      bool value;
      std::string to_sql(ISQLValueRenderer& visitor) const final { return visitor.render(*this); }
    };

    // "relation"."column"
    struct ColumnReference : SingleValue {
      virtual ~ColumnReference() {}

      std::string relation;
      std::string column;

      std::string to_sql(ISQLValueRenderer& visitor) const final { return visitor.render(*this); }
    };

    // function(arguments...)
    struct Aggregate : SingleValue {
      virtual ~Aggregate() {}

      std::string function;
      std::vector<std::unique_ptr<SingleValue>> arguments;

      std::string to_sql(ISQLValueRenderer& visitor) const final { return visitor.render(*this); }
    };

    // (elements...)
    struct List : SingleValue {
      virtual ~List() {}
      std::vector<std::unique_ptr<SingleValue>> elements;

      std::string to_sql(ISQLValueRenderer& visitor) const final { return visitor.render(*this); }
    };

    // CASE value
    // WHEN a THEN a_v
    // WHEN b THEN b_v
    // ELSE otherwise
    // END
    struct CaseSimple : SingleValue {
      std::unique_ptr<SingleValue> value;
      struct When {
        std::unique_ptr<SingleValue> when;
        std::unique_ptr<SingleValue> then;
      };
      std::unique_ptr<SingleValue> otherwise;

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
    struct Case : SingleValue {
      struct When {
        std::unique_ptr<Condition> cond;
        std::unique_ptr<SingleValue> then;
      };
      std::unique_ptr<SingleValue> otherwise;

      std::string to_sql(ISQLValueRenderer& visitor) const final { return visitor.render(*this); }
    };

    // NOT (subcondition)
    struct NotCondition : Condition {
      virtual ~NotCondition() {}

      std::unique_ptr<Condition> subcondition;

      std::string to_sql(ISQLValueRenderer& visitor) const final { return visitor.render(*this); }
    };

    // value op
    struct UnaryCondition : Condition {
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
      std::unique_ptr<SingleValue> value;
      Cond op;

      std::string to_sql(ISQLValueRenderer& visitor) const final { return visitor.render(*this); }
    };

    // lhs op rhs
    struct BinaryCondition : Condition {
      virtual ~BinaryCondition() {}
      enum Cond {
        Eq,
        NotEq,
        LessThan,
        GreaterThan,
        LessThanOrEq,
        GreatherThanOrEq,
        In,
        NotIn,
        IsDistinctFrom,
        IsNotDistinctFrom,
      };
      std::unique_ptr<SingleValue> lhs;
      std::unique_ptr<SingleValue> rhs;
      Cond op;

      std::string to_sql(ISQLValueRenderer& visitor) const final { return visitor.render(*this); }
    };

    // value BETWEEN lower_bound AND upper_bound
    struct BetweenCondition : Condition {
      virtual ~BetweenCondition() {}
      std::unique_ptr<SingleValue> value;
      std::unique_ptr<SingleValue> lower_bound;
      std::unique_ptr<SingleValue> upper_bound;

      std::string to_sql(ISQLValueRenderer& visitor) const final { return visitor.render(*this); }
    };

    // (lhs) op (rhs)
    struct LogicalCondition : Condition {
      virtual ~LogicalCondition() {}

      enum Cond {
        AND,
        OR,
      };
      std::unique_ptr<Condition> lhs;
      std::unique_ptr<Condition> rhs;
      Cond op;

      std::string to_sql(ISQLValueRenderer& visitor) const final { return visitor.render(*this); }
    };

    struct IQuery {
      virtual ~IQuery() {}
      virtual std::string to_sql(ISQLQueryRenderer& visitor) const = 0;
    };

    struct Join {
      std::string relation;
      std::string alias;
      std::unique_ptr<Condition> on;
    };

    // SELECT select FROM relation joins WHERE where GROUP BY group ORDER BY order order_descending
    // This is a SingleValue to provide support for subselects.
    struct SelectQuery : SingleValue, IQuery {
      virtual ~SelectQuery() {}
      std::vector<std::unique_ptr<ast::Value>> select;
      std::string relation;
      std::unique_ptr<ast::Condition> where;
      std::vector<std::unique_ptr<Join>> joins;
      std::vector<std::unique_ptr<ast::Value>> group;
      std::vector<std::unique_ptr<ast::SingleValue>> order;
      bool order_descending = false;

      // TODO: Use Maybe
      std::unique_ptr<size_t> limit;
      std::unique_ptr<size_t> offset;

      std::string to_sql(ISQLQueryRenderer& visitor) const final { return visitor.render(*this); }
      std::string to_sql(ISQLValueRenderer& visitor) const final { return visitor.render(*this); }
    };

    struct UpdateQuery : IQuery {
      virtual ~UpdateQuery() {}
      std::string relation;
      std::unique_ptr<ast::Condition> where;
      std::unique_ptr<size_t> limit;
      std::vector<std::string> columns;
      std::vector<std::unique_ptr<SingleValue>> values;

      std::string to_sql(ISQLQueryRenderer& visitor) const final { return visitor.render(*this); }
    };

    struct DeleteQuery : IQuery {
      virtual ~DeleteQuery() {}
      std::string relation;
      std::unique_ptr<ast::Condition> where;
      std::unique_ptr<size_t> limit;

      std::string to_sql(ISQLQueryRenderer& visitor) const final { return visitor.render(*this); }
    };

    struct InsertQuery : IQuery {
      virtual ~InsertQuery() {}
      std::string relation;
      std::vector<std::string> columns;
      std::vector<std::unique_ptr<SingleValue>> values;

      std::string to_sql(ISQLQueryRenderer& visitor) const final { return visitor.render(*this); }
    };
  }
}

#endif // PERSISTENCE_AST_HPP_INCLUDED

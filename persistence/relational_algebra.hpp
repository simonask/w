#pragma once
#ifndef PERSISTENCE_RELATIONAL_ALGEBRA_HPP_INCLUDED
#define PERSISTENCE_RELATIONAL_ALGEBRA_HPP_INCLUDED

#include <persistence/ast.hpp>
#include <wayward/support/cloning_ptr.hpp>

namespace persistence {
  namespace relational_algebra {
    using w::CloningPtr;

    struct Condition {
      CloningPtr<ast::Condition> cond;
      Condition(std::string sql);
      Condition(CloningPtr<ast::Condition> cond) : cond(std::move(cond)) {}
    };

    Condition operator&&(Condition&& lhs, Condition&& rhs);
    Condition operator||(Condition&& lhs, Condition&& rhs);

    struct Value {
      CloningPtr<ast::SingleValue> value;
      Value(CloningPtr<ast::SingleValue> value) : value(std::move(value)) {}
      Value(const Value&) = default;
      Value(Value&&) = default;
      Value(const char* sql);
      Value(std::string sql);

      // Unary conditions:
      Condition is_null();
      Condition is_not_null();
      Condition is_true();
      Condition is_not_true();
      Condition is_false();
      Condition is_not_false();
      Condition is_unknown();
      Condition is_not_unknown();

      // Binary conditions with string literals:
      Condition like(std::string cmp) &&;
      Condition ilike(std::string cmp);

      // Binary conditions:
      Condition in(Value&& other);
      Condition not_in(Value&& other);
      Condition is_distinct_from(Value&& other);
      Condition is_not_distinct_from(Value&& other);
      Condition operator==(Value&& other) &&;
      Condition operator!=(Value&& other) &&;
      Condition operator<(Value&& other) &&;
      Condition operator>(Value&& other) &&;
      Condition operator<=(Value&& other) &&;
      Condition operator>=(Value&& other) &&;
    };

    // Arithmetic:
    Value operator+(Value&& lhs, Value&& rhs);
    Value operator-(Value&& lhs, Value&& rhs);
    Value operator*(Value&& lhs, Value&& rhs);
    Value operator/(Value&& lhs, Value&& rhs);
    Value operator%(Value&& lhs, Value&& rhs);
    Value operator^(Value&& lhs, Value&& rhs);

    struct Projection {
      explicit Projection(std::string relation);
      Projection(const Projection&) = default;
      Projection(Projection&&) = default;

      Projection where(Condition) const&;
      Projection where(Condition) &&;
      Projection order(Value) const&;
      Projection order(Value) &&;
      Projection reverse_order(bool reverse = true) const&;
      Projection reverse_order(bool reverse = true) &&;

      CloningPtr<ast::SelectQuery> query;
    };

    Projection projection(std::string relation);
    Value      column(std::string relation, std::string column);
    Value      aggregate_impl(std::string func, Value* args, size_t num_args);
    Value      literal(std::string str);
    Value      literal(double number);
    Value      sql(std::string sql);
    Condition  sql_condition(std::string sql);
    Condition  negate(Condition&& cond);

    template <typename... Args>
    Value aggregate(std::string func, Args&&... args) {
      std::array<Value, sizeof...(Args)> a = {{{std::move(args)}...}};
      return aggregate_impl(std::move(func), a.data(), sizeof...(Args));
    }
  }
}

#endif // PERSISTENCE_RELATIONAL_ALGEBRA_HPP_INCLUDED

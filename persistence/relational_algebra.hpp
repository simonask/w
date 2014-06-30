#pragma once
#ifndef PERSISTENCE_RELATIONAL_ALGEBRA_HPP_INCLUDED
#define PERSISTENCE_RELATIONAL_ALGEBRA_HPP_INCLUDED

#include <persistence/ast.hpp>
#include <wayward/support/types.hpp>
#include <wayward/support/cloning_ptr.hpp>

namespace persistence {
  namespace relational_algebra {
    using wayward::CloningPtr;
    using wayward::AnyRef;
    using wayward::IType;

    struct SQL {
      std::string sql;
      explicit SQL(std::string sql) : sql(std::move(sql)) {}
      SQL(SQL&&) = default;
      SQL(const SQL&) = default;
    };

    struct Condition {
      CloningPtr<ast::Condition> cond;
      Condition(SQL sql);
      Condition(CloningPtr<ast::Condition> cond) : cond(std::move(cond)) {}
    };

    Condition operator&&(Condition&& lhs, Condition&& rhs);
    Condition operator||(Condition&& lhs, Condition&& rhs);

    struct Value {
      CloningPtr<ast::SingleValue> value;
      Value() {}
      Value(CloningPtr<ast::SingleValue> value) : value(std::move(value)) {}
      Value(SQL sql);
      Value(const Value&) = default;
      Value(Value&&) = default;
      Value(const char* sql);
      Value(std::string sql);
      Value& operator=(Value&&) = default;
      Value& operator=(const Value&) = default;

      // Unary conditions:
      Condition is_null() &&;
      Condition is_not_null() &&;
      Condition is_true() &&;
      Condition is_not_true() &&;
      Condition is_false() &&;
      Condition is_not_false() &&;
      Condition is_unknown() &&;
      Condition is_not_unknown() &&;

      // Binary conditions with string literals:
      Condition like(std::string cmp) &&;
      Condition ilike(std::string cmp) &&;

      // Binary conditions:
      Condition in(Value&& other) &&;
      Condition not_in(Value&& other) &&;
      Condition is_distinct_from(Value&& other) &&;
      Condition is_not_distinct_from(Value&& other) &&;
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

    struct SelectAlias {
      Value value;
      wayward::Maybe<std::string> alias;

      SelectAlias() {}
      SelectAlias(Value value) : value(std::move(value)) {}
      SelectAlias(Value value, std::string alias) : value(std::move(value)), alias(std::move(alias)) {}
      SelectAlias(const SelectAlias&) = default;
      SelectAlias(SelectAlias&&) = default;
    };

    struct Ordering {
      using OrderingType = ast::Ordering::OrderingType;

      Value value;
      OrderingType ordering = ast::Ordering::Ascending;

      Ordering() {}
      Ordering(Value value) : value(std::move(value)) {}
      Ordering(Value value, OrderingType ordering) : value(std::move(value)), ordering(ordering) {}
      Ordering(const Ordering&) = default;
      Ordering(Ordering&&) = default;
    };

    struct Projection {
      Projection() : query(new ast::SelectQuery) {}
      explicit Projection(std::string relation);
      Projection(const Projection&) = default;
      Projection(Projection&&) = default;
      Projection& operator=(const Projection&) = default;
      Projection& operator=(Projection&&) = default;

      Projection where(Condition) const&;
      Projection where(Condition) &&;
      Projection order(std::vector<Ordering>) const&;
      Projection order(std::vector<Ordering>) &&;
      Projection reverse_order(bool reverse = true) const&;
      Projection reverse_order(bool reverse = true) &&;
      Projection join(std::string relation, std::string alias, Condition on, ast::Join::Type) &&;
      Projection cross_join(std::string relation, std::string alias, Condition on) && { return std::move(*this).join(std::move(relation), std::move(alias), std::move(on), ast::Join::Cross); }
      Projection inner_join(std::string relation, std::string alias, Condition on) && { return std::move(*this).join(std::move(relation), std::move(alias), std::move(on), ast::Join::Inner); }
      Projection left_join(std::string relation, std::string alias, Condition on) &&  { return std::move(*this).join(std::move(relation), std::move(alias), std::move(on), ast::Join::LeftOuter); }
      Projection right_join(std::string relation, std::string alias, Condition on) && { return std::move(*this).join(std::move(relation), std::move(alias), std::move(on), ast::Join::RightOuter); }
      Projection full_join(std::string relation, std::string alias, Condition on) &&  { return std::move(*this).join(std::move(relation), std::move(alias), std::move(on), ast::Join::FullOuter); }
      Projection limit(size_t n) const&;
      Projection limit(size_t n) &&;
      Projection offset(size_t n) const&;
      Projection offset(size_t n) &&;
      Projection select(std::vector<SelectAlias>) &&;
      Projection select(std::vector<SelectAlias>) const&;

      CloningPtr<ast::SelectQuery> query;
    };

    Projection projection(std::string relation);
    Value      column(std::string relation, std::string column);
    Value      column(ast::SymbolicRelation, std::string column);
    Value      aggregate_impl(std::string func, Value* args, size_t num_args);
    Value      literal(AnyRef, const IType*);
    Condition  negate(Condition&& cond);
    SQL        sql(std::string sql);


    template <typename T>
    Value literal(T&& lit) {
      using Type = typename wayward::meta::RemoveConstRef<T>::Type;
      return literal(AnyRef{ lit }, wayward::get_type<Type>());
    }

    template <typename... Args>
    Value aggregate(std::string func, Args&&... args) {
      std::array<Value, sizeof...(Args)> a = {{{std::move(args)}...}};
      return aggregate_impl(std::move(func), a.data(), sizeof...(Args));
    }

    struct SymbolicRelationError : wayward::Error {
      SymbolicRelationError(std::string msg) : wayward::Error(std::move(msg)) {}
    };

    struct IResolveSymbolicRelation {
      virtual ~IResolveSymbolicRelation() {}
      virtual std::string relation_for_symbol(ast::SymbolicRelation relation) const = 0;
    };
  }
}

#endif // PERSISTENCE_RELATIONAL_ALGEBRA_HPP_INCLUDED

#pragma once
#ifndef PERSISTENCE_PROJECTION_HPP_INCLUDED
#define PERSISTENCE_PROJECTION_HPP_INCLUDED

#include <persistence/result_set.hpp>
#include <persistence/ast.hpp>
#include <wayward/support/maybe.hpp>
#include <wayward/support/format.hpp>
#include <persistence/column_traits.hpp>
#include <persistence/column_abilities.hpp>
#include <persistence/record_type.hpp>
#include <persistence/connection.hpp>

#include <functional>

namespace persistence {
  /*
    A Projection<T> is an object that can generate an SQL SELECT
    and put the result into a struct of type T.
  */
  template <typename T>
  struct Projection {
    // static_assert(IsPersistenceType<T>::Value, "Cannot create typed projection for this type, because it doesn't support persistence. Use the PERSISTENCE macro to define properties and relations for the type.");
    Projection() : proj{relational_algebra::projection(get_type<T>()->relation())} {}
    ~Projection() {}
    Projection(const Projection<T>& other) = default;
    Projection(Projection<T>&& other) = default;

    // Query interface:
    Projection<T> where(relational_algebra::Condition cond) &&;
    Projection<T> where(relational_algebra::Condition cond) const&;

    // Debugging/Logging:
    std::string to_sql() const;

    // Inspecting results:
    struct iterator;
    using value_type = T;
    iterator begin();
    iterator end();
    void each(std::function<void(T&)> callback);
    T first();
    std::vector<T> all();

    relational_algebra::Projection to_raw_relation_algebra() const { return proj; }
  private:
    explicit Projection(relational_algebra::Projection proj) : proj(std::move(proj)) {}
    relational_algebra::Projection proj;
    void project(size_t row_idx, T& instance);
  };

  template <typename T>
  std::string Projection<T>::to_sql() const {
    // TODO: Handle multiple data stores
    IConnection& conn = persistence::get_connection();
    return conn.to_sql(*proj.query);
  }

  template <typename T>
  Projection<T> Projection<T>::where(relational_algebra::Condition cond) && {
    return Projection<T>(std::move(proj).where(std::move(cond)));
  }

  template <typename T>
  Projection<T> Projection<T>::where(relational_algebra::Condition cond) const& {
    return Projection<T>(proj.where(std::move(cond)));
  }

  template <typename T>
  Projection<T> from() {
    return Projection<T>();
  }

  using relational_algebra::sql;
  using relational_algebra::SQL;

  template <typename T, typename M>
  struct Column : public ColumnAbilities<Column<T, M>, M>
  {
    using Cond = relational_algebra::Condition;

    relational_algebra::Value sql;
    Column(relational_algebra::Value col) : sql{std::move(col)} {}

    // Binary conditions:
    // Condition in(Value&& other) &&;
    // Condition not_in(Value&& other) &&;
    // Condition is_distinct_from(Value&& other) &&;
    // Condition is_not_distinct_from(Value&& other) &&;
    // Cond operator==(SQL sql) const;
    // Cond operator!=(SQL sql) const;
    // Cond operator<(SQL sql) const;
    // Cond operator>(SQL sql) const;
    // Cond operator<=(SQL sql) const;
    // Cond operator>=(SQL sql) const;
  };

  struct UnregisteredPropertyError : std::runtime_error {
    UnregisteredPropertyError(std::string type_name) : std::runtime_error(nullptr) {
      what_ = w::format("Attempted to use unregistered property on type {0}. Use property(member, column) in the PERSISTENCE block for the type to register the property.", type_name);
    }
    const char* what() const noexcept final { return what_.c_str(); }
    std::string what_;
  };

  template <typename T, typename M>
  Column<T,M> column(M T::*member) {
    const RecordType<T>* t = get_type<T>();
    Maybe<std::string> column = t->find_column_by_member_pointer(member);
    if (column) {
      return Column<T,M>{relational_algebra::column(t->relation(), std::move(*column))};
    }
    throw UnregisteredPropertyError(t->name());
  }

  using relational_algebra::column;
}

#endif // PERSISTENCE_PROJECTION_HPP_INCLUDED

#pragma once
#ifndef PERSISTENCE_PROJECTION_HPP_INCLUDED
#define PERSISTENCE_PROJECTION_HPP_INCLUDED

#include <persistence/result_set.hpp>
#include <persistence/ast.hpp>
#include <persistence/relational_algebra.hpp>
#include <wayward/support/maybe.hpp>
#include <wayward/support/format.hpp>
#include <persistence/column_traits.hpp>
#include <persistence/column_abilities.hpp>
#include <persistence/record_type.hpp>
#include <persistence/connection.hpp>
#include <persistence/connection_provider.hpp>

#include <functional>

namespace persistence {
  using relational_algebra::sql;
  using relational_algebra::SQL;
  using wayward::Maybe;
  using wayward::Nothing;

  struct UnregisteredPropertyError : std::runtime_error {
    UnregisteredPropertyError(std::string type_name) : std::runtime_error(nullptr) {
      what_ = wayward::format("Attempted to use unregistered property on type {0}. Use property(member, column) in the PERSISTENCE block for the type to register the property.", type_name);
    }
    const char* what() const noexcept final { return what_.c_str(); }
    std::string what_;
  };

  template <typename T, typename M> struct Column;

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

  /*
    A Projection<T> is an object that can generate an SQL SELECT
    and put the results into a struct of type T.
  */
  template <typename T>
  struct Projection {
    // static_assert(IsPersistenceType<T>::Value, "Cannot create typed projection for this type, because it doesn't support persistence. Use the PERSISTENCE macro to define properties and relations for the type.");
    Projection() : proj{relational_algebra::projection(get_type<T>()->relation())} { init_aliasing(); }
    ~Projection() {}
    Projection(const Projection<T>& other) : proj(other.proj), materialized_(nullptr), select_aliases_(other.select_aliases_) {}
    Projection(Projection<T>&& other) = default;

    // Query interface:
    Projection<T> where(relational_algebra::Condition cond) &&;
    Projection<T> where(relational_algebra::Condition cond) const& { return moved_copy().where(std::move(cond)); }

    template <typename M>
    Projection<T> order(Column<T, M> col) &&;
    template <typename M>
    Projection<T> order(Column<T, M> col) const& { return moved_copy().order(std::move(col)); }
    template <typename M>
    Projection<T> order(M T::*field) && { return std::move(*this).order(column(field)); }
    template <typename M>
    Projection<T> order(M T::*field) const& { return this->order(column(field)); }
    Projection<T> order(SQL sql) &&;
    Projection<T> order(SQL sql) const& { return moved_copy().order(std::move(sql)); }
    Projection<T> reverse_order() &&;
    Projection<T> reverse_order() const& { return moved_copy().reverse_order(); }

    size_t count() const;

    Projection<T> limit(size_t n) && { return Projection<T>(std::move(proj).limit(n), std::move(select_aliases_)); }
    Projection<T> limit(size_t n) const& { return moved_copy().limit(n); }
    Projection<T> offset(size_t n) && { return Projection<T>(std::move(proj).offset(n), std::move(select_aliases_)); }
    Projection<T> offset(size_t n) const& { return moved_copy().offset(n); }

    // Debugging/Logging:
    std::string to_sql() const;

    // Inspecting results:
    void each(std::function<void(T&)> callback);
    Maybe<T> first();
    std::vector<T> all();

    relational_algebra::Projection to_raw_relation_algebra() const { return proj; }
  private:
    using SelectAliasMap = std::map<const IPropertyOf<T>*, std::string>;
    explicit Projection(relational_algebra::Projection proj, SelectAliasMap aliases = SelectAliasMap()) : proj(std::move(proj)), select_aliases_(std::move(aliases)) {}
    relational_algebra::Projection proj;
    std::unique_ptr<IResultSet> materialized_;
    SelectAliasMap select_aliases_;

    void materialize();
    void project(size_t row_idx, T& instance) const;
    void init_aliasing();

    Projection<T> moved_copy() const {
      Projection<T> copy(*this);
      return std::move(copy);
    }
  };

  template <typename T>
  void Projection<T>::init_aliasing() {
    // TODO: Support joins.
    const IRecordType* type = get_type<T>();
    for (size_t i = 0; i < type->num_properties(); ++i) {
      auto& property = type->property_at(i);
      std::string alias = wayward::format("t0_c{0}", i);
      proj.query->select.push_back({wayward::make_cloning_ptr(new ast::ColumnReference(proj.query->relation, property.column())), alias});
      select_aliases_[dynamic_cast<const IPropertyOf<T>*>(&property)] = alias;
    }
  }

  template <typename T>
  std::string Projection<T>::to_sql() const {
    auto conn = current_connection_provider().acquire_connection_for_data_store(get_type<T>()->data_store());
    return conn.to_sql(*proj.query);
  }

  template <typename T>
  Projection<T> Projection<T>::where(relational_algebra::Condition cond) && {
    return Projection<T>(std::move(proj).where(std::move(cond)), std::move(select_aliases_));
  }

  template <typename T>
  template <typename M>
  Projection<T> Projection<T>::order(Column<T, M> col) && {
    return Projection<T>(std::move(proj).order({std::move(col.sql)}), std::move(select_aliases_));
  }

  template <typename T>
  Projection<T> Projection<T>::reverse_order() && {
    return Projection<T>(std::move(proj).reverse_order(), std::move(select_aliases_));
  }

  template <typename T>
  void Projection<T>::each(std::function<void(T&)> callback) {
    T tmp_;
    materialize();
    if (materialized_ != nullptr) {
      for (size_t i = 0; i < materialized_->height(); ++i) {
        project(i, tmp_);
        callback(tmp_);
      }
    }
  }

  template <typename T>
  std::vector<T> Projection<T>::all() {
    std::vector<T> records;
    materialize();
    if (materialized_ != nullptr) {
      size_t n = materialized_->height();
      records.resize(n);
      for (size_t i = 0; i < n; ++i) {
        project(i, records[i]);
      }
    }
    return std::move(records);
  }

  template <typename T>
  Maybe<T> Projection<T>::first() {
    if (materialized_) {
      if (materialized_->height() > 0) {
        T tmp_;
        project(0, tmp_);
        return std::move(tmp_);
      }
    } else {
      auto limited = limit(1);
      auto v = limited.all();
      if (v.size() > 0) {
        return std::move(v.front());
      }
    }
    return Nothing;
  }

  template <typename T>
  void Projection<T>::materialize() {
    if (!materialized_) {
      auto conn = current_connection_provider().acquire_connection_for_data_store(get_type<T>()->data_store());
      materialized_ = conn.execute(*proj.query);
    }
  }

  template <typename T>
  void Projection<T>::project(size_t row_idx, T& record) const {
    if (materialized_) {
      for (auto& pair: select_aliases_) {
        pair.first->set(record, *materialized_, row_idx, pair.second);
      }
      get_type<T>()->initialize_associations_in_object(&record);
    }
  }

  template <typename T>
  Projection<T> from() {
    return Projection<T>();
  }
}

#endif // PERSISTENCE_PROJECTION_HPP_INCLUDED

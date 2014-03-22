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
  using relational_algebra::sql;
  using relational_algebra::SQL;
  using w::Maybe;
  using w::Nothing;

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
    Projection(const Projection<T>& other) = default;
    Projection(Projection<T>&& other) = default;

    // Query interface:
    Projection<T> where(relational_algebra::Condition cond) &&;
    Projection<T> where(relational_algebra::Condition cond) const&;

    template <typename M>
    Projection<T> order(Column<T, M> col) &&;
    template <typename M>
    Projection<T> order(Column<T, M> col) const&;
    Projection<T> order(SQL sql) &&;
    Projection<T> order(SQL sql) const&;

    size_t count() const;

    Projection<T> limit(size_t n) && { return std::move(proj).limit(n); }
    Projection<T> limit(size_t n) const& { return proj.limit(n); }
    Projection<T> offset(size_t n) && { return std::move(proj).offset(n); }
    Projection<T> offset(size_t n) const& { return proj.offset(n); }

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
    std::unique_ptr<IResultSet> realized_;
    SelectAliasMap select_aliases_;

    void realize();
    void project(size_t row_idx, T& instance) const;
    void init_aliasing();
  };

  template <typename T>
  void Projection<T>::init_aliasing() {
    // TODO: Support joins.
    const IRecordType* type = get_type<T>();
    for (size_t i = 0; i < type->num_properties(); ++i) {
      auto& property = type->property_at(i);
      std::string alias = w::format("t0_c{0}", i);
      proj.query->select.push_back(ast::ColumnAlias{proj.query->relation, property.column(), alias});
      select_aliases_[dynamic_cast<const IPropertyOf<T>*>(&property)] = alias;
    }
  }

  template <typename T>
  std::string Projection<T>::to_sql() const {
    // TODO: Handle multiple data stores
    IConnection& conn = persistence::get_connection();
    return conn.to_sql(*proj.query);
  }

  template <typename T>
  Projection<T> Projection<T>::where(relational_algebra::Condition cond) && {
    return Projection<T>(std::move(proj).where(std::move(cond)), std::move(select_aliases_));
  }

  template <typename T>
  Projection<T> Projection<T>::where(relational_algebra::Condition cond) const& {
    return Projection<T>(proj.where(std::move(cond)), select_aliases_);
  }

  template <typename T>
  void Projection<T>::each(std::function<void(T&)> callback) {
    T tmp_;
    realize();
    if (realized_ != nullptr) {
      for (size_t i = 0; i < realized_->height(); ++i) {
        project(i, tmp_);
        callback(tmp_);
      }
    }
  }

  template <typename T>
  Maybe<T> Projection<T>::first() {
    if (realized_) {
      if (realized_->height() > 0) {
        T tmp_;
        project(0, tmp_);
        return std::move(tmp_);
      }
    } else {
      auto limited = limit(1);
      auto v = limited.all();
      if (v.size() > 0) {
        return std::move(v.first());
      }
    }
    return Nothing;
  }

  template <typename T>
  void Projection<T>::realize() {
    if (!realized_) {
      realized_ = persistence::get_connection().execute(*proj.query);
    }
  }

  template <typename T>
  void Projection<T>::project(size_t row_idx, T& record) const {
    if (realized_) {
      for (auto& pair: select_aliases_) {
        pair.first->set(record, *realized_, row_idx, pair.second);
      }
      get_type<T>()->initialize_associations_in_object(&record);
    }
  }

  template <typename T>
  Projection<T> from() {
    return Projection<T>();
  }

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

#pragma once
#ifndef PERSISTENCE_PROJECTION_HPP_INCLUDED
#define PERSISTENCE_PROJECTION_HPP_INCLUDED

#include <persistence/result_set.hpp>
#include <persistence/ast.hpp>

namespace persistence {

  /*
    A Projection<T> is an object that can generate an SQL SELECT
    and put the result into a struct of type T.
  */
  template <typename T>
  struct Projection {
    ~Projection() {}
    Projection(const Projection<T>& other) = default;
    Projection(Projection<T>&& other) = default;

    // Query interface:
    Projection<T> where(std::string conditions) const; // TODO: Conditions class
    Projection<T> select(std::string fields) const; // TODO: List of columns
    Projection<T> joins(std::string joins) const; // TODO: Associations
    Projection<T> order(std::string order_by_clause) const; // TODO: Column
    Projection<T> reverse_order() const;
    Projection<T> limit(size_t n) const;
    Projection<T> offset(size_t n) const;
    size_t count() const;

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
  private:
    Projection();
    Query sql_;
    void project(size_t row_idx, T& instance);
  };
}

#endif // PERSISTENCE_PROJECTION_HPP_INCLUDED

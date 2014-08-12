#pragma once
#ifndef PERSISTENCE_COLUMN_HPP_INCLUDED
#define PERSISTENCE_COLUMN_HPP_INCLUDED

#include <wayward/support/maybe.hpp>
#include <wayward/support/type.hpp>

#include <persistence/property.hpp>
#include <persistence/relational_algebra.hpp>
#include <persistence/column_abilities.hpp>
#include <persistence/record_type.hpp>

namespace persistence {
  struct UnregisteredPropertyError : wayward::Error {
    UnregisteredPropertyError(const std::string& type_name);
  };

  template <typename Type, typename ColumnType>
  struct Column : ColumnAbilities<Column<Type, ColumnType>, ColumnType> {
    std::string column_name;
    Maybe<std::string> explicit_alias;

    Column(ColumnType Type::*member) {
      const RecordType<Type>* t = get_type<Type>();
      Maybe<std::string> column = t->find_column_by_member_pointer(member);
      if (column) {
        column_name = *column;
      } else {
        throw UnregisteredPropertyError(t->name());
      }
    }

    Column(std::string relation_alias, ColumnType Type::*member) : explicit_alias(std::move(relation_alias)) {
      const RecordType<Type>* t = get_type<Type>();
      Maybe<std::string> column = t->find_column_by_member_pointer(member);
      if (column) {
        column_name = *column;
      } else {
        throw UnregisteredPropertyError(t->name());
      }
    }

    Column(std::string column_name) : column_name(std::move(column_name)) {}
    Column(std::string relation_alias, std::string column_name) : column_name(std::move(column_name)), explicit_alias(std::move(relation_alias)) {}

    relational_algebra::Value value() const& {
      if (explicit_alias) {
        return relational_algebra::column(*explicit_alias, column_name);
      } else {
        return relational_algebra::column(reinterpret_cast<ast::SymbolicRelation>(get_type<Type>()), column_name);
      }
    }

    relational_algebra::Value value() && {
      if (explicit_alias) {
        return relational_algebra::column(std::move(*explicit_alias), std::move(column_name));
      } else {
        return relational_algebra::column(reinterpret_cast<ast::SymbolicRelation>(get_type<Type>()), std::move(column_name));
      }
    }
  };

  template <typename Type, typename ColumnType>
  Column<Type, ColumnType> column(ColumnType Type::*member) {
    return Column<Type, ColumnType>(member);
  }
  template <typename Type, typename ColumnType>
  Column<Type, ColumnType> column(std::string alias, ColumnType Type::*member) {
    return Column<Type, ColumnType>(std::move(alias), member);
  }
  template <typename Type, typename ColumnType>
  Column<Type, ColumnType> column(std::string column) {
    return Column<Type, ColumnType>(std::move(column));
  }
  template <typename Type, typename ColumnType>
  Column<Type, ColumnType> column(std::string alias, std::string column) {
    return Column<Type, ColumnType>(std::move(alias), std::move(column));
  }

  template <typename Type, typename ColumnType, typename Other>
  auto eq(ColumnType Type::*a, Other b) -> decltype(column(a) == b) {
    return column(a) == b;
  }

  template <typename Type, typename ColumnType, typename Other>
  auto eq(Other b, ColumnType Type::*a) -> decltype(column(a) == b) {
    return column(a) == b;
  }
}

#endif // PERSISTENCE_COLUMN_HPP_INCLUDED

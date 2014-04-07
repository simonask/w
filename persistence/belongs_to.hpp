#pragma once
#ifndef PERSISTENCE_BELONGS_TO_HPP_INCLUDED
#define PERSISTENCE_BELONGS_TO_HPP_INCLUDED

#include <persistence/association.hpp>
#include <persistence/primary_key.hpp>
#include <persistence/column_abilities.hpp>

namespace persistence {
  template <typename AssociatedType>
  struct BelongsTo {
    const IAssociationTo<AssociatedType>* association_ = nullptr;
    PrimaryKey id;
  };

  template <typename O, typename A>
  struct BelongsToAssociation : Association<O, A> {
    using MemberPointer = BelongsTo<A> O::*;
    explicit BelongsToAssociation(std::string key, MemberPointer ptr) : Association<O, A>{std::move(key)}, ptr_(ptr) {}
    MemberPointer ptr_;

    void initialize_in_object(O& object) const final {
      (object.*ptr_).association_ = this;
    }
  };

  template <typename Col, typename T>
  struct ColumnAbilities<Col, BelongsTo<T>>: LiteralEqualityAbilities<Col, std::int64_t> {};

  template <typename T>
  struct BelongsToType : IDataTypeFor<BelongsTo<T>> {
    std::string name() const final { return wayward::format("BelongsTo<{0}>", get_type<T>()->name()); }
    bool is_nullable() const final { return false; }

    bool has_value(const BelongsTo<T>& value) const final {
      return value.id.is_persisted();
    }

    void extract_from_results(BelongsTo<T>& value, const IResultSet& r, size_t row, const std::string& col) const final {
      get_type<PrimaryKey>()->extract_from_results(value.id, r, row, col);
    }
  };

  template <typename T>
  const BelongsToType<T>* build_type(const TypeIdentifier<BelongsToType<T>>*) {
    static const auto p = new BelongsToType<T>;
    return p;
  }
}

#endif // PERSISTENCE_BELONGS_TO_HPP_INCLUDED

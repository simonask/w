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

  template <typename T>
  struct ColumnAbilities<BelongsTo<T>>: LiteralEqualityAbilities<std::int64_t> {};

  template <typename T>
  struct BelongsToType : IType {
    std::string name() const final { return w::format("BelongsTo<{0}>", get_type<T>()->name()); }
    bool is_nullable() const final { return false; }
  };

  template <typename T>
  struct BuildType<BelongsTo<T>> {
    static const BelongsToType<T>* build() {
      static const auto p = new BelongsToType<T>;
      return p;
    }
  };
}

#endif // PERSISTENCE_BELONGS_TO_HPP_INCLUDED

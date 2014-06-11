#pragma once
#ifndef PERSISTENCE_HAS_MANY_HPP_INCLUDED
#define PERSISTENCE_HAS_MANY_HPP_INCLUDED

#include <persistence/association.hpp>

namespace persistence {
  template <typename AssociatedType>
  struct HasMany : IPluralAssociationFieldTo<AssociatedType> {
    const IAssociationTo<AssociatedType>* association_ = nullptr;
    Maybe<std::vector<RecordPtr<AssociatedType>>> records_;

    const IRecordType& foreign_type() const final {
      return *get_type<AssociatedType>();
    }

    void populate(std::vector<RecordPtr<AssociatedType>> records) final {
      records_ = std::move(records);
    }

    bool is_populated() const final {
      return (bool)records_;
    }
  };

  template <typename O, typename A>
  struct HasManyAssociation : PluralAssociation<O, A> {
    using MemberPointer = HasMany<A> O::*;
    explicit HasManyAssociation(std::string key, MemberPointer ptr) : PluralAssociation<O, A>{std::move(key)}, ptr_(ptr) {}
    MemberPointer ptr_;

    void initialize_in_object(O& object, Context* ctx) const final {
      (object.*ptr_).association_ = this;
    }

    IPluralAssociationField* get_field(O& object) const final {
      return &(object.*ptr_);
    }

    const IPluralAssociationField* get_field(const O& object) const final {
      return &(object.*ptr_);
    }
  };
}

#endif // PERSISTENCE_HAS_MANY_HPP_INCLUDED

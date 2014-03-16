#pragma once
#ifndef PERSISTENCE_HAS_MANY_HPP_INCLUDED
#define PERSISTENCE_HAS_MANY_HPP_INCLUDED

#include <persistence/association.hpp>

namespace persistence {
  template <typename AssociatedType>
  struct HasMany {
    const IAssociationTo<AssociatedType>* association_ = nullptr;
  };

  template <typename O, typename A>
  struct HasManyAssociation : Association<O, A> {
    using MemberPointer = HasMany<A> O::*;
    explicit HasManyAssociation(std::string key, MemberPointer ptr) : Association<O, A>{std::move(key)}, ptr_(ptr) {}
    MemberPointer ptr_;

    void initialize_in_object(O& object) const final {
      (object.*ptr_).association_ = this;
    }
  };
}

#endif // PERSISTENCE_HAS_MANY_HPP_INCLUDED

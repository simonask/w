#pragma once
#ifndef PERSISTENCE_HAS_ONE_HPP_INCLUDED
#define PERSISTENCE_HAS_ONE_HPP_INCLUDED

#include <persistence/association.hpp>

namespace persistence {
  template <typename AssociatedType>
  struct HasOne {
    const IAssociationTo<AssociatedType>* association_ = nullptr;
  };

  template <typename O, typename A>
  struct HasOneAssociation : Association<O, A> {
    using MemberPointer = HasOne<A> O::*;
    explicit HasOneAssociation(std::string key, MemberPointer ptr) : Association<O, A>{std::move(key)}, ptr_(ptr) {}
    MemberPointer ptr_;

    void initialize_in_object(O& object) const final {
      (object.*ptr_).association_ = this;
    }
  };
}

#endif // PERSISTENCE_HAS_ONE_HPP_INCLUDED

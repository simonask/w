#pragma once
#ifndef PERSISTENCE_HAS_ONE_HPP_INCLUDED
#define PERSISTENCE_HAS_ONE_HPP_INCLUDED

#include <persistence/association.hpp>

namespace persistence {
  template <typename AssociatedType>
  struct HasOne : SingularAssociationAnchor<AssociatedType> {
  };

  template <typename O, typename A>
  struct HasOneAssociation : SingularAssociationBase<O, HasOne<A>> {
    using MemberPointer = HasOne<A> O::*;
    explicit HasOneAssociation(std::string key, MemberPointer ptr) : SingularAssociationBase<O, HasOne<A>>{std::move(key), ptr} {}
  };
}

#endif // PERSISTENCE_HAS_ONE_HPP_INCLUDED

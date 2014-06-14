#pragma once
#ifndef PERSISTENCE_HAS_MANY_HPP_INCLUDED
#define PERSISTENCE_HAS_MANY_HPP_INCLUDED

#include <persistence/association.hpp>

namespace persistence {
  template <typename AssociatedType>
  struct HasMany : PluralAssociationAnchor<AssociatedType> {
    Maybe<std::vector<RecordPtr<AssociatedType>>> records_;

    void populate(std::vector<RecordPtr<AssociatedType>> records) final {
      records_ = std::move(records);
    }

    void load() final {
      // TODO!
    }

    std::vector<RecordPtr<AssociatedType>> get() final {
      return records_ ? *records_ : std::vector<RecordPtr<AssociatedType>>{};
    }

    bool is_loaded() const final {
      return (bool)records_;
    }
  };

  template <typename O, typename A>
  struct HasManyAssociation : PluralAssociationBase<O, HasMany<A>> {
    using MemberPointer = HasMany<A> O::*;
    explicit HasManyAssociation(std::string key, MemberPointer ptr) : PluralAssociationBase<O, HasMany<A>>{std::move(key), ptr} {}
  };
}

#endif // PERSISTENCE_HAS_MANY_HPP_INCLUDED

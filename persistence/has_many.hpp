#pragma once
#ifndef PERSISTENCE_HAS_MANY_HPP_INCLUDED
#define PERSISTENCE_HAS_MANY_HPP_INCLUDED

#include <persistence/association.hpp>
#include <persistence/projection.hpp>

namespace persistence {
  template <typename AssociatedType>
  struct HasMany : PluralAssociationAnchor<AssociatedType> {
    Maybe<std::vector<RecordPtr<AssociatedType>>> records_;
    std::function<PrimaryKey(const HasMany<AssociatedType>&)> get_id_of_owner_;

    void populate(std::vector<RecordPtr<AssociatedType>> records) final {
      records_ = std::move(records);
    }

    PrimaryKey id_of_owner() const {
      return get_id_of_owner_(*this);
    }

    Projection<AssociatedType> scope() const {
      auto pk = id_of_owner();
      if (!pk.is_persisted()) {
        throw AssociationError{"Cannot load HasMany association for an unsaved record."};
      }
      return persistence::from<AssociatedType>(*this->context_).where(column<AssociatedType, int64_t>(this->association()->foreign_key()) == pk);
    }

    void load() final {
      if (!records_) {
        records_ = scope().all();
      }
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
    explicit HasManyAssociation(MemberPointer ptr, std::string name, std::string fkey) : PluralAssociationBase<O, HasMany<A>>{ptr, std::move(name), std::move(fkey)} {}

    size_t member_offset(O& object) const {
      return (size_t)(((char*)&(object.*(this->member_))) - (char*)&object);
    }

    void initialize_in_object(O& object, Context* context) const override {
      PluralAssociationBase<O, HasMany<A>>::initialize_in_object(object, context);

      // This is sinful (and undefined behaviour, and probably slow:
      // The aim is to be able to go from pointer to HasMany<A> to pointer to O.
      size_t offset = member_offset(object);
      this->get(object)->get_id_of_owner_ = [=](const HasMany<A>& anchor) -> PrimaryKey {
        const O* obj = reinterpret_cast<const O*>(reinterpret_cast<const char*>(&anchor) - offset);
        auto t = get_type<O>()->primary_key();
        auto p = dynamic_cast<const PropertyOfBase<O, PrimaryKey>*>(t);
        if (p == nullptr) {
          throw AssociationError{"Owner of HasMany association has a primary key that isn't a PrimaryKey property."};
        }
        return p->get_known(*obj);
      };
    }
  };
}

#endif // PERSISTENCE_HAS_MANY_HPP_INCLUDED

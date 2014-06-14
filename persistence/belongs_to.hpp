#pragma once
#ifndef PERSISTENCE_BELONGS_TO_HPP_INCLUDED
#define PERSISTENCE_BELONGS_TO_HPP_INCLUDED

#include <persistence/association.hpp>
#include <persistence/primary_key.hpp>
#include <persistence/column_abilities.hpp>

#include <wayward/support/either.hpp>

namespace persistence {
  using wayward::Either;

  struct Context;

  template <typename T>
  RecordPtr<T> find_by_primary_key(Context& ctx, PrimaryKey key);

  template <typename T>
  const PrimaryKey* get_pk_for_record(const RecordPtr<T>& record) {
    auto pk = get_type<T>()->primary_key();
    if (pk) {
      auto pk_typed = dynamic_cast<const PropertyOf<T, PrimaryKey>*>(pk);
      if (pk_typed) {
        auto& pk_id = pk_typed->get(*record);
        if (pk_id.is_persisted()) {
          return &pk_id;
        }
      }
    }
    return nullptr;
  }

  template <typename T>
  struct BelongsTo : SingularAssociationAnchor<T> {
    using AssociatedType = T;

    BelongsTo() : value_(PrimaryKey{}) {}
    Either<PrimaryKey, RecordPtr<T>> value_;

    bool operator==(const RecordPtr<T>& rhs) const { return get() == rhs; }
    bool operator!=(const RecordPtr<T>& rhs) const { return !(*this == rhs); }

    BelongsTo<T>& operator=(RecordPtr<T> ptr) {
      value_ = ptr;
      return *this;
    }

    BelongsTo<T>& operator=(int64_t id) {
      value_ = PrimaryKey{id};
      return *this;
    }

    PrimaryKey id() const {
      auto ptr = id_ptr();
      if (ptr) {
        return *ptr;
      } else {
        return PrimaryKey{};
      }
    }

    const PrimaryKey* id_ptr() const {
      const PrimaryKey* ptr = nullptr;

      value_.template when<PrimaryKey>([&](const PrimaryKey& key) {
        if (key.is_persisted()) {
          ptr = &key;
        }
      });

      value_.template when<RecordPtr<T>>([&](const RecordPtr<T>& referenced) {
        if (referenced) {
          ptr = get_pk_for_record(referenced);
        }
      });

      return ptr;
    }

    RecordPtr<T> operator->() {
      load();
      return get();
    }

    RecordPtr<T> operator->() const {
      return get();
    }

    bool is_set() const {
      bool b = false;
      value_.template when<PrimaryKey>([&](const PrimaryKey& key) {
        b = key.is_persisted();
      });
      value_.template when<RecordPtr<T>>([&](const RecordPtr<T>& ptr) {
        b = ptr != nullptr;
      });
      return b;
    }


    /// IAssociationAnchor interface:

    bool is_loaded() const final {
      return value_.template is_a<RecordPtr<T>>();
    }

    void load() final {
      value_.template when<PrimaryKey>([&](const PrimaryKey& key) {
        if (key.is_persisted()) {
          value_ = find_by_primary_key<T>(*this->context_, key);
        }
      });
    }


    /// ISingularAssociationAnchor<T> interface:

    void populate(RecordPtr<T> ptr) final {
      value_ = std::move(ptr);
    }

    RecordPtr<T> get() {
      load();
      RecordPtr<T> ptr;
      value_.template when<RecordPtr<T>>([&](const RecordPtr<T>& p) {
        ptr = p;
      });
      return std::move(ptr);
    }

    RecordPtr<T> get() const {
      RecordPtr<T> ptr;
      value_.template when<RecordPtr<T>>([&](const RecordPtr<T>& record) {
        ptr = record;
      });
      return std::move(ptr);
    }
  };

  template <typename O, typename A>
  struct BelongsToAssociation : SingularAssociationBase<O, BelongsTo<A>> {
    using MemberPointer = BelongsTo<A> O::*;
    explicit BelongsToAssociation(std::string key, MemberPointer ptr) : SingularAssociationBase<O, BelongsTo<A>>(std::move(key), ptr) {}
  };

  template <typename Col, typename T>
  struct ColumnAbilities<Col, BelongsTo<T>>: LiteralEqualityAbilities<Col, std::int64_t> {};

  struct IBelongsToType : IType {
    virtual ~IBelongsToType() {}
  };

  template <typename T>
  struct BelongsToType : IDataTypeFor<BelongsTo<T>, IBelongsToType> {
    std::string name() const final { return wayward::format("BelongsTo<{0}>", get_type<T>()->name()); }
    bool is_nullable() const final { return false; }

    bool has_value(const BelongsTo<T>& value) const final {
      return value.id().is_persisted();
    }

    bool deserialize_value(BelongsTo<T>& value, const wayward::data_franca::ScalarSpectator& source) const final {
      PrimaryKey key;
      if (get_type<PrimaryKey>()->deserialize_value(key, source)) {
        value.value_ = std::move(key);
        return true;
      }
      return false;
    }

    bool serialize_value(const BelongsTo<T>& value, wayward::data_franca::ScalarMutator& target) const final {
      return get_type<PrimaryKey>()->serialize_value(value.id(), target);
    }
  };

  template <typename T>
  const BelongsToType<T>* build_type(const TypeIdentifier<BelongsTo<T>>*) {
    static const auto p = new BelongsToType<T>;
    return p;
  }

  /*
    We're specializing the Property class for BelongsTo associations, because
    when seeing the association as a property (i.e., when dealing with it in SQL),
    it should be a primary key, rather than the object data that data_franca adapters
    perceive it to be.
  */
  template <typename T, typename M>
  struct PropertyOf<T, BelongsTo<M>> : PropertyOfBase<T, BelongsTo<M>> {
    using MemberPtr = typename PropertyOfBase<T, BelongsTo<M>>::MemberPtr;
    PropertyOf(MemberPtr ptr, std::string col) : PropertyOfBase<T, BelongsTo<M>>(ptr, std::move(col)) {}

    DataRef
    get_data(const T& record) const override {
      auto& assoc = this->get(record);
      auto id = assoc.id_ptr();
      if (id) {
        return DataRef{ *id };
      } else {
        return DataRef{ Nothing };
      }
    }
  };
}

#endif // PERSISTENCE_BELONGS_TO_HPP_INCLUDED

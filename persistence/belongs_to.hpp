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
    auto pk = dynamic_cast<const IPropertyOf<T>*>(get_type<T>()->primary_key());
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

  template <typename AssociatedType>
  struct BelongsTo : ISingularAssociationFieldTo<AssociatedType> {
    using Type = AssociatedType;
    const ISingularAssociationTo<AssociatedType>* association_ = nullptr;

    BelongsTo() : value_(PrimaryKey{}) {}

    Context* ctx_ = nullptr;
    Either<PrimaryKey, RecordPtr<AssociatedType>> value_;

    bool operator==(const RecordPtr<AssociatedType>& rhs) const { return get() == rhs; }
    bool operator!=(const RecordPtr<AssociatedType>& rhs) const { return !(*this == rhs); }

    BelongsTo<AssociatedType>& operator=(RecordPtr<AssociatedType> ptr) {
      value_ = ptr;
      return *this;
    }

    BelongsTo<AssociatedType>& operator=(int64_t id) {
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

      value_.template when<RecordPtr<AssociatedType>>([&](const RecordPtr<AssociatedType>& referenced) {
        if (referenced) {
          ptr = get_pk_for_record(referenced);
        }
      });

      return ptr;
    }

    RecordPtr<AssociatedType> operator->() {
      return get();
    }

    RecordPtr<AssociatedType> operator->() const {
      return get();
    }

    void load() {
      value_.template when<PrimaryKey>([&](const PrimaryKey& key) {
        if (key.is_persisted()) {
          value_ = find_by_primary_key<AssociatedType>(*ctx_, key);
        }
      });
    }

    // ISingularAssociationTo<> interface
    bool is_populated() const final {
      return value_.template is_a<RecordPtr<AssociatedType>>();
    }

    void populate(RecordPtr<AssociatedType> ptr) final {
      value_ = std::move(ptr);
    }

    bool is_set() const final {
      bool b = false;
      value_.template when<PrimaryKey>([&](const PrimaryKey& key) {
        b = key.is_persisted();
      });
      value_.template when<RecordPtr<AssociatedType>>([&](const RecordPtr<AssociatedType>& ptr) {
        b = ptr != nullptr;
      });
      return b;
    }

    RecordPtr<AssociatedType> get() final {
      load();
      RecordPtr<AssociatedType> ptr;
      value_.template when<RecordPtr<AssociatedType>>([&](const RecordPtr<AssociatedType>& p) {
        ptr = p;
      });
      return std::move(ptr);
    }

    RecordPtr<AssociatedType> get() const {
      RecordPtr<AssociatedType> ptr;
      value_.template when<RecordPtr<AssociatedType>>([&](const RecordPtr<AssociatedType>& record) {
        ptr = record;
      });
      return std::move(ptr);
    }

    const IRecordType& foreign_type() const final {
      return *get_type<AssociatedType>();
    }
  };

  template <typename O, typename A>
  struct BelongsToAssociation : SingularAssociation<O, A> {
    using MemberPointer = BelongsTo<A> O::*;
    explicit BelongsToAssociation(std::string key, MemberPointer ptr) : SingularAssociation<O, A>{std::move(key)}, ptr_(ptr) {}
    MemberPointer ptr_;

    void initialize_in_object(O& object, Context* ctx) const final {
      (object.*ptr_).ctx_ = ctx;
      (object.*ptr_).association_ = this;
    }

    ISingularAssociationField* get_field(O& object) const final {
      return &(object.*ptr_);
    }

    const ISingularAssociationField* get_field(const O& object) const final {
      return &(object.*ptr_);
    }
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

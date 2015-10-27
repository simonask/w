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
  RecordPtr<T> find(Context& ctx, PrimaryKey key);

  template <typename T>
  const PrimaryKey* get_pk_for_record(const RecordPtr<T>& record) {
    auto pk = get_type<T>()->primary_key();
    if (pk) {
      auto pk_typed = dynamic_cast<const PropertyOf<T, PrimaryKey>*>(pk);
      if (pk_typed) {
        auto& pk_id = pk_typed->get_known(*record);
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
          value_ = persistence::find<T>(*this->context_, key);
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
    explicit BelongsToAssociation(MemberPointer ptr, std::string name, std::string fkey) : SingularAssociationBase<O, BelongsTo<A>>(ptr, std::move(name), std::move(fkey)) {}
  };

  template <typename Col, typename T>
  struct ColumnAbilities<Col, BelongsTo<T>>: LiteralEqualityAbilities<Col, std::int64_t> {};

  struct IBelongsToType : IType {
    virtual ~IBelongsToType() {}
  };

  template <typename T>
  struct BelongsToType : wayward::DataTypeFor<BelongsTo<T>, IBelongsToType> {
    std::string name() const final { return wayward::format("BelongsTo<{0}>", get_type<T>()->name()); }
    bool is_nullable() const final { return false; }

    bool has_value(const BelongsTo<T>& value) const final {
      return value.id().is_persisted();
    }

    void visit(BelongsTo<T>& value, wayward::DataVisitor& visitor) const final {
      if (visitor.can_modify() && !visitor.is_nil_at_current()) {
        auto pk = value.id();
        get_type<decltype(pk)>()->visit_data(pk, visitor);
        value = pk;
      } else {
        auto ptr = value.id_ptr();
        if (ptr) {
          get_type<decltype(*ptr)>()->visit_data(*ptr, visitor);
        } else {
          visitor(Nothing);
        }
      }
    }
  };

  template <typename T>
  const BelongsToType<T>* build_type(const wayward::TypeIdentifier<BelongsTo<T>>*) {
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

    Result<Any> get(AnyConstRef record) const override {
      if (!record.is_a<T>()) {
        return detail::make_type_error_for_mismatching_record_type(get_type<T>(), record.type_info());
      }
      auto object = record.get<T>();
      auto& assoc = this->get_known(*object);
      return Any{assoc};
    }
  };
}

#endif // PERSISTENCE_BELONGS_TO_HPP_INCLUDED

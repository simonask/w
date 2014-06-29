#pragma once
#ifndef PERSISTENCE_PRIMARY_KEY_HPP_INCLUDED
#define PERSISTENCE_PRIMARY_KEY_HPP_INCLUDED

#include <cstdint>
#include <wayward/support/type.hpp>
#include <persistence/column_abilities.hpp>
#include <persistence/result_set.hpp>

#include <wayward/support/monad.hpp>
#include <wayward/support/types.hpp>

namespace persistence {
  using int64 = std::int64_t;

  struct PrimaryKey {
    PrimaryKey() {}
    PrimaryKey(int64 id) : id(id) {}
    PrimaryKey(const PrimaryKey& other) = default;
    PrimaryKey& operator=(const PrimaryKey& other) = default;
    operator int64() const { return id; }

    bool is_persisted() const { return id > 0; }
  //private:
    int64 id = -1;
  };

  struct PrimaryKeyType : wayward::DataTypeFor<PrimaryKey> {
    std::string name() const final { return "PrimaryKey"; }
    bool is_nullable() const final { return false; }

    bool has_value(const PrimaryKey& value) const final {
      return value.is_persisted();
    }

    void visit(PrimaryKey& pk, wayward::DataVisitor& visitor) const final {
      if (visitor.can_modify()) {
        visitor(pk.id);
      } else {
        if (has_value(pk)) {
          visitor(pk.id);
        } else {
          visitor.visit_nil();
        }
      }
    }
  };

  const PrimaryKeyType* build_type(const wayward::TypeIdentifier<PrimaryKey>*);

  template <typename Col>
  struct ColumnAbilities<Col, PrimaryKey> : LiteralEqualityAbilities<Col, int64> {};
}

namespace wayward {
  namespace monad {
    template <> struct Join<persistence::PrimaryKey> {
      using Type = persistence::PrimaryKey;
    };
    template <> struct Join<Maybe<persistence::PrimaryKey>> {
      using Type = persistence::PrimaryKey;
    };

    template <>
    struct Bind<persistence::PrimaryKey> {
      template <typename F>
      static auto bind(persistence::PrimaryKey k, F f) -> typename Join<Maybe<decltype(f(std::declval<int64_t>()))>>::Type {
        if (k.is_persisted()) {
          return f(k.id);
        }
        return Nothing;
      }
    };
  }
}

#endif // PERSISTENCE_PRIMARY_KEY_HPP_INCLUDED

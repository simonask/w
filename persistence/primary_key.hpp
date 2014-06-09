#pragma once
#ifndef PERSISTENCE_PRIMARY_KEY_HPP_INCLUDED
#define PERSISTENCE_PRIMARY_KEY_HPP_INCLUDED

#include <cstdint>
#include <persistence/type.hpp>
#include <persistence/column_abilities.hpp>
#include <persistence/result_set.hpp>

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

  struct PrimaryKeyType : IDataTypeFor<PrimaryKey> {
    std::string name() const final { return "PrimaryKey"; }
    bool is_nullable() const final { return false; }

    bool has_value(const PrimaryKey& value) const final {
      return value.is_persisted();
    }

    void extract_from_results(PrimaryKey& value, const IResultSet& r, size_t row, const std::string& col) const final {
      std::stringstream ss;
      ss.str(r.get(row, col));
      int64_t v;
      ss >> v;
      value = PrimaryKey{v};
    }
  };

  const PrimaryKeyType* build_type(const TypeIdentifier<PrimaryKey>*);

  template <typename Col>
  struct ColumnAbilities<Col, PrimaryKey> : LiteralEqualityAbilities<Col, int64> {};
}

#endif // PERSISTENCE_PRIMARY_KEY_HPP_INCLUDED

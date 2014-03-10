#pragma once
#ifndef PERSISTENCE_PRIMARY_KEY_HPP_INCLUDED
#define PERSISTENCE_PRIMARY_KEY_HPP_INCLUDED

#include <cstdint>

namespace persistence {
  using int64 = std::int64_t;

  struct PrimaryKey {
    PrimaryKey() {}
    PrimaryKey(int64 id) : id(id) {}
    PrimaryKey(const PrimaryKey& other) = default;
    PrimaryKey& operator=(const PrimaryKey& other) = default;
    operator int64() const { return id; }

    bool is_persisted() const { return id > 0; }
  private:
    int64 id = -1;
  };
}

#endif // PERSISTENCE_PRIMARY_KEY_HPP_INCLUDED

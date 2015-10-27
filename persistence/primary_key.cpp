#include "persistence/primary_key.hpp"
#include "persistence/property.hpp"

namespace persistence {
  const PrimaryKeyType* build_type(const wayward::TypeIdentifier<PrimaryKey>*) {
    static const PrimaryKeyType* p = new PrimaryKeyType;
    return p;
  }
}

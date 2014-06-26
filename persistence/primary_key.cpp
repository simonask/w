#include <persistence/primary_key.hpp>

namespace persistence {
  const PrimaryKeyType* build_type(const TypeIdentifier<PrimaryKey>*) {
    static const PrimaryKeyType* p = new PrimaryKeyType;
    return p;
  }
}

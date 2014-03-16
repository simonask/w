#include <persistence/primary_key.hpp>

namespace persistence {
  const PrimaryKeyType* BuildType<PrimaryKey>::build() {
    static const PrimaryKeyType* p = new PrimaryKeyType;
    return p;
  }
}

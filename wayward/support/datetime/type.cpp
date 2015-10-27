#include "wayward/support/datetime/type.hpp"

namespace wayward {
  const DateTimeType* build_type(const TypeIdentifier<DateTime>*) {
    static DateTimeType* t = new DateTimeType;
    return t;
  }
}

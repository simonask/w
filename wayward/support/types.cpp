#include "wayward/support/types.hpp"

namespace wayward {
  const NothingTypeType* build_type(const TypeIdentifier<NothingType>*) {
    static const NothingTypeType* p = new NothingTypeType;
    return p;
  }

  const StringType* build_type(const TypeIdentifier<std::string>*) {
    static const StringType* p = new StringType;
    return p;
  }

  #define DEFINE_NUMERIC_TYPE(T) \
    const NumericType<T>* build_type(const TypeIdentifier<T>*) { \
      static const NumericType<T>* p = new NumericType<T>{#T}; \
      return p; \
    }

  DEFINE_NUMERIC_TYPE(std::int32_t)
  DEFINE_NUMERIC_TYPE(std::int64_t)
  DEFINE_NUMERIC_TYPE(std::uint32_t)
  DEFINE_NUMERIC_TYPE(std::uint64_t)
  DEFINE_NUMERIC_TYPE(float)
  DEFINE_NUMERIC_TYPE(double)

  namespace detail {
    std::string maybe_type_name(const IType* inner_type) {
      return wayward::format("Maybe<{0}>", inner_type->name());
    }
  }
}

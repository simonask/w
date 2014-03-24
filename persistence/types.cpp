#include <persistence/types.hpp>
#include <wayward/support/format.hpp>

namespace persistence {
  const StringType* BuildType<std::string>::build() {
    static const StringType* p = new StringType;
    return p;
  }

  #define DEFINE_NUMERIC_TYPE(T) \
    const NumericType<T>* BuildType<T>::build() { \
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

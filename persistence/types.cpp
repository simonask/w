#include <persistence/types.hpp>
#include <wayward/support/format.hpp>

namespace persistence {
  const StringType* BuildType<std::string>::build() {
    static const StringType* p = new StringType;
    return p;
  }

  #define DEFINE_NUMERIC_TYPE(T) \
    const NumericType* BuildType<T>::build() { \
      static const NumericType* p = new NumericType{#T, sizeof(T)*8, std::is_signed<T>::value, std::is_floating_point<T>::value}; \
      return p; \
    }

  DEFINE_NUMERIC_TYPE(std::int32_t)
  DEFINE_NUMERIC_TYPE(std::int64_t)
  DEFINE_NUMERIC_TYPE(std::uint32_t)
  DEFINE_NUMERIC_TYPE(std::uint64_t)
  DEFINE_NUMERIC_TYPE(float)
  DEFINE_NUMERIC_TYPE(double)

  std::string MaybeType::name() const {
    return w::format("Maybe<{0}>", inner_type_->name());
  }
}

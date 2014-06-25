#include "persistence/property.hpp"

#include "wayward/support/format.hpp"

namespace persistence {
  namespace detail {
    using wayward::make_error;

    wayward::ErrorPtr make_type_error_for_mismatching_record_type(const IType* expected_type, const TypeInfo& got_type) {
      return make_error<TypeError>(wayward::format("Cannot access property of object of type '{0}'. This property belongs to objects of type '{1}'.", got_type.name(), expected_type->name()));
    }

    wayward::ErrorPtr make_type_error_for_mismatching_value_type(const IType* record_type, const IType* expected_type, const TypeInfo& got_type) {
      return make_error<TypeError>(wayward::format("Cannot assign property of type '{0}' with value of type '{1}'.", expected_type->name(), got_type.name()));
    }
  }
}

#include "persistence/column.hpp"

namespace persistence {
  UnregisteredPropertyError::UnregisteredPropertyError(const std::string& type_name)
  : wayward::Error{wayward::format("Attempted to use unregistered property on type {0}. Use property(member, column) in the PERSISTENCE block for the type to register the property.", type_name)}
  {}
}

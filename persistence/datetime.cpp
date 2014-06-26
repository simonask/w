#include <persistence/datetime.hpp>
#include <persistence/result_set.hpp>

#include <stdio.h>

namespace persistence {
  using wayward::DateTime;
  using wayward::TypeIdentifier;

  namespace relational_algebra {
    Value RepresentAsLiteral<DateTime>::literal(const DateTime& dt) {
      return Value{make_cloning_ptr(new persistence::ast::StringLiteral{dt.iso8601()})};
    }
  }
}

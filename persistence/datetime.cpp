#include <persistence/datetime.hpp>
#include <persistence/result_set.hpp>

namespace persistence {
  using wayward::DateTime;

  const DateTimeType* build_type(const TypeIdentifier<DateTime>*) {
    static DateTimeType* t = new DateTimeType;
    return t;
  }

  void DateTimeType::extract_from_results(DateTime& value, const IResultSet& rs, size_t row, const std::string& col) const {
    auto string = rs.get(row, col);

    // TODO: Handle time zones!
    auto m = DateTime::strptime(string, "%Y-%m-%d %H:%M:%s");
    if (m) {
      value = *m;
    }
  }

  namespace relational_algebra {
    Value RepresentAsLiteral<DateTime>::literal(const DateTime& dt) {
      return Value{make_cloning_ptr(new persistence::ast::StringLiteral{dt.iso8601()})};
    }
  }
}

#include <persistence/datetime.hpp>
#include <persistence/result_set.hpp>

#include <wayward/support/data_franca/spelunker.hpp>
#include <wayward/support/data_franca/mutator.hpp>

namespace persistence {
  using wayward::DateTime;

  const DateTimeType* build_type(const TypeIdentifier<DateTime>*) {
    static DateTimeType* t = new DateTimeType;
    return t;
  }

  bool DateTimeType::deserialize_value(DateTime& value, const wayward::data_franca::ScalarSpelunker& source) const {
    std::string string_rep;
    if (source >> string_rep) {
      auto m = DateTime::strptime(string_rep, "%Y-%m-%d %H:%M:%s");
      if (m) {
        value = *m;
        return true;
      }
    }
    return false;
  }

  bool DateTimeType::serialize_value(const DateTime& value, wayward::data_franca::ScalarMutator& target) const {
    target << value.strftime("%Y-%m-%d %H:%M:%s");
    return true;
  }

  namespace relational_algebra {
    Value RepresentAsLiteral<DateTime>::literal(const DateTime& dt) {
      return Value{make_cloning_ptr(new persistence::ast::StringLiteral{dt.iso8601()})};
    }
  }
}

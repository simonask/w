#include <persistence/datetime.hpp>
#include <persistence/result_set.hpp>
#include <persistence/property.hpp>

#include <wayward/support/data_franca/spectator.hpp>
#include <wayward/support/data_franca/mutator.hpp>

#include <stdio.h>

namespace persistence {
  using wayward::DateTime;


  // Result<void> DateTimeType::deserialize_value(DateTime& value, const wayward::data_franca::ScalarSpectator& source) const {
  //   std::string string_rep;
  //   if (source >> string_rep) {
  //     // PostgreSQL timestamp with time zone looks like this: YYYY-mm-dd HH:MM:ss+ZZ
  //     // Unfortunately, POSIX strptime can't deal with the two-digit timezone at the end, so we tinker with the string
  //     // to get it into a parseable state.
  //     std::string local_time_string = string_rep.substr(0, 19);
  //     std::string timezone_string = string_rep.substr(string_rep.size() - 3);
  //     local_time_string += timezone_string;
  //     if (timezone_string.size() == 3) {
  //       local_time_string += "00";
  //     }

  //     auto m = DateTime::strptime(local_time_string, "%Y-%m-%d %T%z");

  //     if (m) {
  //       value = std::move(*m);
  //       return Nothing;
  //     }
  //     return wayward::make_error<wayward::Error>(wayward::format("Couldn't parse DateTime from string: '{0}'", string_rep));
  //   }
  //   return wayward::make_error<wayward::Error>("ERROR: Couldn't parse DateTime (input not a string).");
  // }

  // Result<void> DateTimeType::serialize_value(const DateTime& value, wayward::data_franca::ScalarMutator& target) const {
  //   std::stringstream ss { value.strftime("%Y-%m-%d %T%z") };
  //   target << ss.str();
  //   return Nothing;
  // }

  // ast::Ptr<ast::SingleValue> DateTimeType::make_literal(AnyConstRef data) const {
  //   if (!data.is_a<DateTime>()) {
  //     throw TypeError("DateTimeType::make_literal called with a value that isn't a DateTime.");
  //   }
  //   auto& dt = *data.get<const DateTime&>();
  //   return make_cloning_ptr(new persistence::ast::StringLiteral{dt.iso8601()});
  // }
}

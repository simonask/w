#include "wayward/support/datetime/type.hpp"

#include <wayward/support/data_franca/spectator.hpp>
#include <wayward/support/data_franca/mutator.hpp>

namespace wayward {
  const DateTimeType* build_type(const TypeIdentifier<DateTime>*) {
    static DateTimeType* t = new DateTimeType;
    return t;
  }

  bool DateTimeType::deserialize_value(DateTime& value, const wayward::data_franca::ScalarSpectator& source) const {
    std::string string_rep;
    if (source >> string_rep) {
      // PostgreSQL timestamp with time zone looks like this: YYYY-mm-dd HH:MM:ss+ZZ
      // Unfortunately, POSIX strptime can't deal with the two-digit timezone at the end, so we tinker with the string
      // to get it into a parseable state.
      std::string local_time_string = string_rep.substr(0, 19);
      std::string timezone_string = string_rep.substr(string_rep.size() - 3);
      local_time_string += timezone_string;
      if (timezone_string.size() == 3) {
        local_time_string += "00";
      }

      auto m = DateTime::strptime(local_time_string, "%Y-%m-%d %T%z");

      if (m) {
        value = std::move(*m);
        return true;
      } else {
        fprintf(stderr, "ERROR: Couldn't parse DateTime: %s\n", local_time_string.c_str());
      }
    }
    return false;
  }

  bool DateTimeType::serialize_value(const DateTime& value, wayward::data_franca::ScalarMutator& target) const {
    std::stringstream ss { value.strftime("%Y-%m-%d %T%z") };
    target << ss.str();
    return true;
  }
}

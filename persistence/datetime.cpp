#include <persistence/datetime.hpp>
#include <persistence/result_set.hpp>

#include <wayward/support/data_franca/spectator.hpp>
#include <wayward/support/data_franca/mutator.hpp>

#include <stdio.h>

namespace persistence {
  using wayward::DateTime;

  const DateTimeType* build_type(const TypeIdentifier<DateTime>*) {
    static DateTimeType* t = new DateTimeType;
    return t;
  }

  bool DateTimeType::deserialize_value(DateTime& value, const wayward::data_franca::ScalarSpectator& source) const {
    std::string string_rep;
    if (source >> string_rep) {
      // PostgreSQL timestamp with time zone looks like this: YYYY-mm-dd HH:MM:ss+ZZ
      // Unfortunately, POSIX strptime can't deal with the two-digit timezone at the end, so we parse that bit manually.

      std::string local_time_string = string_rep.substr(0, 19);
      auto m = DateTime::strptime(local_time_string, "%Y-%m-%d %T");
      if (m) {
        auto local_time = std::move(*m);
        std::string timezone_indicator = string_rep.substr(string_rep.size() - 3);
        std::stringstream ss(timezone_indicator);
        int tz_hours {0};
        ss >> tz_hours;
        wayward::Seconds tz_seconds {tz_hours * 3600};
        local_time.timezone_ = wayward::Timezone{tz_seconds, false};
        value = std::move(local_time);
        return true;
      }
    }
    return false;
  }

  bool DateTimeType::serialize_value(const DateTime& value, wayward::data_franca::ScalarMutator& target) const {
    std::stringstream ss { value.strftime("%Y-%m-%d %T") };

    // TODO: Use format() once it can do advanced integer formatting.
    char buffer[4];
    sprintf(buffer, "%+2lld", value.timezone_.utc_offset.repr_.count() / 3600);
    ss << buffer;

    target << ss.str();
    return true;
  }

  namespace relational_algebra {
    Value RepresentAsLiteral<DateTime>::literal(const DateTime& dt) {
      return Value{make_cloning_ptr(new persistence::ast::StringLiteral{dt.iso8601()})};
    }
  }
}

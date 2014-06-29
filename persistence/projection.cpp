#include "persistence/projection.hpp"

#include <wayward/support/format.hpp>

namespace persistence {

  RelationProjector::RelationProjector(std::string relation_alias, const IRecordType* type)
  : record_type_(type), relation_alias_(std::move(relation_alias))
  {
    // Build column aliases:
    auto len = record_type_->num_properties();
    for (size_t i = 0; i < record_type_->num_properties(); ++i) {
      auto prop = record_type_->abstract_property_at(i);
      auto alias = wayward::format("{0}_{1}", relation_alias_, prop->column());
      column_aliases_[prop->column()] = std::move(alias);
    }
  }

  void RelationProjector::add_join(const IAssociation* association, CloningPtr<RelationProjector> other) {
    sub_projectors_[association] = std::move(other);
  }

  void RelationProjector::rebuild_joins(std::map<std::string, RelationProjector*>& out_joins) {
    out_joins[relation_alias_] = this;
    for (auto& pair: sub_projectors_) {
      pair.second->rebuild_joins(out_joins);
    }
  }

  void RelationProjector::append_selects(std::vector<relational_algebra::SelectAlias>& out_selects) const {
    // Append our own columns:
    for (auto& pair: column_aliases_) {
      out_selects.emplace_back(relational_algebra::column(relation_alias_, pair.first), pair.second);
    }

    // Append columns for all subprojectors:
    for (auto& pair: sub_projectors_) {
      pair.second->append_selects(out_selects);
    }
  }

  namespace {
    using wayward::DataVisitor;
    using wayward::DateTime;

    struct ColumnProjectionVisitor : DataVisitor {
      const IResultSet& results;
      size_t row;
      const std::string& column_alias;

      ColumnProjectionVisitor(const IResultSet& results, size_t row, const std::string& column_alias) : results(results), row(row), column_alias(column_alias) {}

      void visit_nil() final {}

      void visit_boolean(bool& value) final {
        auto v = results.get(row, column_alias);
        value = v ? (*v == "t") : false;
      }

      void visit_int8(std::int8_t& value) final {
        auto v = results.get(row, column_alias);
        if (!v) return;
        std::stringstream ss{*v};
        ss >> value;
      }

      void visit_int16(std::int16_t& value) final {
        auto v = results.get(row, column_alias);
        if (!v) return;
        std::stringstream ss{*v};
        ss >> value;
      }

      void visit_int32(std::int32_t& value) final {
        auto v = results.get(row, column_alias);
        if (!v) return;
        std::stringstream ss{*v};
        ss >> value;
      }

      void visit_int64(std::int64_t& value) final {
        auto v = results.get(row, column_alias);
        if (!v) return;
        std::stringstream ss{*v};
        ss >> value;
      }

      void visit_uint8(std::uint8_t& value) final {
        auto v = results.get(row, column_alias);
        if (!v) return;
        std::stringstream ss{*v};
        ss >> value;
      }

      void visit_uint16(std::uint16_t& value) final {
        auto v = results.get(row, column_alias);
        if (!v) return;
        std::stringstream ss{*v};
        ss >> value;
      }

      void visit_uint32(std::uint32_t& value) final {
        auto v = results.get(row, column_alias);
        if (!v) return;
        std::stringstream ss{*v};
        ss >> value;
      }

      void visit_uint64(std::uint64_t& value) final {
        auto v = results.get(row, column_alias);
        if (!v) return;
        std::stringstream ss{*v};
        ss >> value;
      }

      void visit_float(float& value) final {
        auto v = results.get(row, column_alias);
        if (!v) return;
        std::stringstream ss{*v};
        ss >> value;
      }

      void visit_double(double& value) final {
        auto v = results.get(row, column_alias);
        if (!v) return;
        std::stringstream ss{*v};
        ss >> value;
      }

      void visit_string(std::string& value) final {
        auto v = results.get(row, column_alias);
        if (!v) return;
        value = *v;
      }

      void visit_key_value(const std::string& key, AnyRef data, const IType* type) final {
        throw TypeError{"Key-values cannot be decoded from SQL rows."};
      }

      void visit_element(std::int64_t idx, AnyRef data, const IType* type) final {
        throw TypeError("Lists cannot be decoded from SQL rows.");
      }

      void visit_special(AnyRef data, const IType* type) final {
        if (data.is_a<DateTime>()) {
          auto v = results.get(row, column_alias);
          if (v) {
            auto& value = *data.get<DateTime&>();

            std::string string_rep = std::move(*v);
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
            }
            throw TypeError(wayward::format("Couldn't parse DateTime from string: '{0}'", string_rep));
          }
          throw TypeError("ERROR: Couldn't parse DateTime (input not a string).");
        }
      }

      bool can_modify() const final {
        return true;
      }

      bool is_nil_at_current() const final {
        return results.is_null_at(row, column_alias);
      }
    };

    struct RecordProjectionVisitor : DataVisitor {
      const RelationProjector::ColumnAliases& aliases;
      const IResultSet& results;
      size_t row;

      RecordProjectionVisitor(const RelationProjector::ColumnAliases& aliases, const IResultSet& results, size_t row) : aliases(aliases), results(results), row(row) {}

      void unsupported() { throw TypeError{"Unsupported operation."}; }

      void visit_nil() final { unsupported(); }
      void visit_boolean(bool&) final { unsupported(); }
      void visit_int8(std::int8_t&) final { unsupported(); }
      void visit_int16(std::int16_t&) final { unsupported(); }
      void visit_int32(std::int32_t&) final { unsupported(); }
      void visit_int64(std::int64_t&) final { unsupported(); }
      void visit_uint8(std::uint8_t&) final { unsupported(); }
      void visit_uint16(std::uint16_t&) final { unsupported(); }
      void visit_uint32(std::uint32_t&) final { unsupported(); }
      void visit_uint64(std::uint64_t&) final { unsupported(); }
      void visit_float(float&) final { unsupported(); }
      void visit_double(double&) final { unsupported(); }
      void visit_string(std::string&) final { unsupported(); }
      void visit_element(std::int64_t idx, AnyRef data, const IType* type) final { unsupported(); }
      void visit_special(AnyRef data, const IType* type) final { unsupported(); }
      bool can_modify() const final { return true; }
      bool is_nil_at_current() const final { return false; }

      void visit_key_value(const std::string& key, AnyRef data, const IType* type) final {
        auto it = aliases.find(key);
        if (it == aliases.end()) {
          return;
        }

        // TODO: Get a backend-specific column converter!
        ColumnProjectionVisitor visitor { results, row, it->second };
        type->visit_data(data, visitor);
      }
    };
  }

  void RelationProjector::populate_with_results(Context& ctx, AnyRef record_ref, const IResultSet& results, size_t row) {
    RecordProjectionVisitor visitor { column_aliases_, results, row };
    record_type_->visit_data(record_ref, visitor);

    for (auto& pair: sub_projectors_) {
      auto& anchor = *pair.first->get_anchor(record_ref);
      pair.second->project_and_populate_association(ctx, anchor, results, row);
    }
  }
}

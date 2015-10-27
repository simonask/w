#include "persistence/projection.hpp"
#include "persistence/data_store.hpp"
#include "persistence/connection_pool.hpp"

#include <wayward/support/format.hpp>


namespace persistence {
  namespace detail {
    struct ProjectionBase::Private
     : wayward::Cloneable<ProjectionBase::Private, relational_algebra::IResolveSymbolicRelation>
     {
      CloningPtr<RelationProjector> base_projector;

      // The map of joins is alias=>projector, and it needs to be rebuilt every time we make a copy of this Projection.
      // Ownership of the detail::RelationProjector pointer is held by the hierarchy of projectors.
      // We keep track of these to find the proper detail::RelationProjector on which to add a join.
      std::map<std::string, RelationProjector*> join_map;

      // An unnamed relation is a relation that is joined upon another without an alias.
      // From the AST perspective, these are what ast::SymbolicRelation refer to.
      // We keep track of this because it would be annoyingly verbose to have to refer to the relation alias
      // in queries for every referenced column -- it is the rare case that it's ambiguous after all.
      std::map<const IRecordType*, std::string> first_relations_;

      std::string relation_for_symbol(ast::SymbolicRelation relation) const final {
        auto t = reinterpret_cast<const IRecordType*>(relation);
        auto it = first_relations_.find(t);
        if (it != first_relations_.end()) {
          return it->second;
        } else {
          throw relational_algebra::SymbolicRelationError(wayward::format("Could not find relation for symbol {0}. Internal consistency error.", relation));
        }
      }
    };

    ProjectionBase::ProjectionBase(Context& ctx, CloningPtr<RelationProjector> base) : context_(ctx), private_(new Private) {
      private_->join_map[base->relation_alias()] = base.get();
      auto real_table_name = base->record_type()->relation();
      projection_.query->relation = real_table_name;
      if (real_table_name != base->relation_alias()) {
        projection_.query->relation_alias = base->relation_alias();
      }
      private_->first_relations_[base->record_type()] = base->relation_alias();
      private_->base_projector = std::move(base);
    }

    ProjectionBase::ProjectionBase(const ProjectionBase& other)
    : context_(other.context_)
    , projection_(other.projection_)
    , results_(nullptr)
    , private_(other.private_)
    {
      // We were copied, so pointer values have changed.
      rebuild_join_map();
    }

    ProjectionBase::ProjectionBase(ProjectionBase&& other)
    : context_(other.context_)
    , projection_(std::move(other.projection_))
    , results_(std::move(other.results_))
    , private_(std::move(other.private_))
    {}

    ProjectionBase::ProjectionBase(ProjectionBase&& other, relational_algebra::Projection new_projection)
    : context_(other.context_)
    , projection_(std::move(new_projection))
    , results_(nullptr)
    , private_(std::move(other.private_))
    {}

    ProjectionBase::~ProjectionBase()
    {}

    ProjectionBase& ProjectionBase::operator=(const ProjectionBase& other) {
      projection_ = other.projection_;
      results_ = nullptr;
      private_ = other.private_;
      // We were copied, so pointer values have changed.
      rebuild_join_map();
      return *this;
    }

    ProjectionBase& ProjectionBase::operator=(ProjectionBase&& other) {
      projection_ = std::move(other.projection_);
      results_ = std::move(other.results_);
      private_ = std::move(other.private_);
      return *this;
    }

    const IRecordType* ProjectionBase::primary_type() const {
      return private_->base_projector->record_type();
    }

    RelationProjector* ProjectionBase::primary_projector() const {
      return private_->base_projector.get();
    }

    std::string ProjectionBase::alias_for(const IRecordType* type) const {
      auto it = private_->first_relations_.find(type);
      if (it != private_->first_relations_.end()) {
        return it->second;
      }
      assert(false); // LOGIC ERROR!
    }

    std::string ProjectionBase::to_sql() {
      update_select_expressions();
      auto conn = current_connection_provider().acquire_connection_for_data_store(primary_type()->data_store());
      return conn.to_sql(*projection_.query, *private_);
    }

    size_t ProjectionBase::count() {
      if (results_)
        return results_->height();

      auto p_copy = projection_.select({
        {relational_algebra::aggregate("COUNT", relational_algebra::column(private_->base_projector->relation_alias(), private_->base_projector->record_type()->abstract_primary_key()->column())),
        "count"}
      });
      auto conn = current_connection_provider().acquire_connection_for_data_store(primary_type()->data_store());
      auto results = conn.execute(*p_copy.query, *private_);
      uint64_t count = 0;
      Maybe<std::string> count_column = results->get(0, "count");
      std::stringstream ss(*count_column);
      ss >> count;
      return count;
    }

    void ProjectionBase::rebuild_join_map() {
      private_->join_map.clear();
      private_->base_projector->rebuild_join_map_recursively(private_->join_map);
    }

    void ProjectionBase::execute_query() {
      if (results_ == nullptr) {
        update_select_expressions();
        auto conn = current_connection_provider().acquire_connection_for_data_store(primary_type()->data_store());
        //conn.logger()->log(wayward::Severity::Debug, "p", wayward::format("Load {0}", get_type<Primary>()->name()));
        results_ = conn.execute(*projection_.query, *private_);
      }
    }

    void ProjectionBase::update_select_expressions() {
      std::vector<relational_algebra::SelectAlias> selects;
      private_->base_projector->append_selects(selects);
      projection_ = std::move(projection_).select(std::move(selects));
    }


    void ProjectionBase::build_join(std::string from_alias, const IRecordType* from_type, const IAssociation& assoc, CloningPtr<RelationProjector> projector, ast::Join::Type type) {
      // Look up where to hook up the join, and perform some sanity checks on the way:
      auto it = private_->join_map.find(from_alias);
      if (it == private_->join_map.end()) {
        throw AssociationError(wayward::format("Unknown relation '{0}'.", from_alias));
      }
      auto& source_projector = it->second;

      // Hook it up in the projector hierarchy:
      auto target_type = projector->record_type();
      auto& to_alias = projector->relation_alias();

      // Add it to the list of joins:
      private_->join_map[to_alias] = projector.get();
      source_projector->add_join(assoc, std::move(projector));

      // If this is the first join with this relation, update the first_relations list:
      if (private_->first_relations_.find(target_type) == private_->first_relations_.end()) {
        private_->first_relations_[target_type] = to_alias;
      }

      // Prepare information for the JOIN condition:
      std::string relation = target_type->relation();

      // Build the condition with the relational algebra DSL:
      auto lhs = relational_algebra::column(from_alias, assoc.foreign_key());
      auto rhs = relational_algebra::column(to_alias, target_type->abstract_primary_key()->column());
      auto cond = (std::move(lhs) == std::move(rhs));

      switch (type) {
        case ast::Join::Inner:      projection_ = std::move(projection_).inner_join(std::move(relation), std::move(to_alias), std::move(cond)); break;
        case ast::Join::LeftOuter:  projection_ = std::move(projection_).left_join(std::move(relation), std::move(to_alias), std::move(cond)); break;
        case ast::Join::RightOuter: projection_ = std::move(projection_).right_join(std::move(relation), std::move(to_alias), std::move(cond)); break;
        default: assert(false); // NIY
      }
    }



    void throw_association_type_mismatch_error(const IRecordType* expected, const IRecordType* got) {
      throw AssociationTypeMismatchError{wayward::format("Could not populate association expecting type {0} with object of type {1}.", expected->name(), got->name())};
    }

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

    void RelationProjector::add_join(const IAssociation& association, CloningPtr<RelationProjector> other) {
      sub_projectors_[&association] = std::move(other);
    }

    void RelationProjector::rebuild_join_map_recursively(std::map<std::string, RelationProjector*>& out_joins) {
      out_joins[relation_alias_] = this;
      for (auto& pair: sub_projectors_) {
        pair.second->rebuild_join_map_recursively(out_joins);
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
                return;
              }
              throw TypeError(wayward::format("Couldn't parse DateTime from string: '{0}'", string_rep));
            }
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
}

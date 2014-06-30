#pragma once
#ifndef PERSISTENCE_PROJECTION_HPP_INCLUDED
#define PERSISTENCE_PROJECTION_HPP_INCLUDED

#include <wayward/support/maybe.hpp>

#include <persistence/result_set.hpp>
#include <persistence/column.hpp>
#include <persistence/ast.hpp>
#include <persistence/relational_algebra.hpp>
#include <persistence/column_traits.hpp>
#include <persistence/column_abilities.hpp>
#include <persistence/record_type.hpp>
#include <persistence/connection.hpp>
#include <persistence/connection_provider.hpp>
#include <persistence/belongs_to.hpp>
#include <persistence/context.hpp>

#include <wayward/support/meta.hpp>
#include <wayward/support/error.hpp>
#include <wayward/support/logger.hpp>
#include <wayward/support/data_franca/adapters.hpp>
#include <wayward/support/types.hpp>

#include <functional>
#include <cassert>

namespace persistence {
  namespace meta = ::wayward::meta;
  using relational_algebra::sql;
  using relational_algebra::SQL;
  using wayward::Maybe;
  using wayward::Nothing;
  using wayward::CloningPtr;

  struct AssociationTypeMismatchError : wayward::Error {
    AssociationTypeMismatchError(std::string msg) : Error(std::move(msg)) {}
  };

  template <typename... Relations> struct Joins;

  template <typename T, typename Jx = Joins<>> struct Projection;

  namespace detail {
    struct RelationProjector {
      using ColumnAliases = std::map<std::string, std::string>; // original->alias

      RelationProjector(std::string relation_alias, const IRecordType* record_type);
      const IRecordType* record_type() const { return record_type_; }
      const std::string& relation_alias() const { return relation_alias_; }

      virtual RelationProjector* clone() const = 0;

      void add_join(const IAssociation&, CloningPtr<RelationProjector>);
      void rebuild_join_map_recursively(std::map<std::string, RelationProjector*>& out_joins);
      void append_selects(std::vector<relational_algebra::SelectAlias>& out_selects) const;

      void populate_with_results(Context&, AnyRef record_ref, const IResultSet&, size_t row);

      virtual void project_and_populate_association(Context&, IAssociationAnchor&, const IResultSet& result_set, size_t row) = 0;
    private:
      const IRecordType* record_type_;
      std::string relation_alias_;
      std::map<const IAssociation*, CloningPtr<RelationProjector>> sub_projectors_;
      ColumnAliases column_aliases_;
    };

    void throw_association_type_mismatch_error(const IRecordType* expected, const IRecordType* got);

    template <typename T>
    struct RelationProjectorFor final : wayward::Cloneable<RelationProjectorFor<T>, RelationProjector> {
      RelationProjectorFor(std::string relation_alias)
      : wayward::Cloneable<RelationProjectorFor<T>, RelationProjector>(std::move(relation_alias), wayward::get_type<T>())
      {}
      RelationProjectorFor() : RelationProjectorFor(get_type<T>()->relation()) {}

      RecordPtr<T> project(Context& ctx, const IResultSet& result_set, size_t row) {
        auto record = ctx.create<T>();
        this->populate_with_results(ctx, *record, result_set, row);
        return std::move(record);
      }

      void project_and_populate_association(Context& ctx, IAssociationAnchor& association, const IResultSet& result_set, size_t row) {
        auto ptr = project(ctx, result_set, row);
        auto typed_association = dynamic_cast<ISingularAssociationAnchor<T>*>(&association);
        if (typed_association == nullptr) {
          throw_association_type_mismatch_error(association.association()->foreign_type(), get_type<T>());
        }
        typed_association->populate(std::move(ptr));
      }
    };

    struct ProjectionBase {
      ~ProjectionBase();

      const IRecordType* primary_type() const;

      std::string to_sql();
      size_t count();
    protected:
      ProjectionBase(const ProjectionBase&);
      ProjectionBase(ProjectionBase&&);
      ProjectionBase& operator=(const ProjectionBase&);
      ProjectionBase& operator=(ProjectionBase&&);

      ProjectionBase(Context& ctx, CloningPtr<RelationProjector> base_projector);
      ProjectionBase(ProjectionBase&& other, relational_algebra::Projection new_projection);

      Context& context_;
      relational_algebra::Projection projection_;
      std::unique_ptr<IResultSet> results_;

      struct Private;
      CloningPtr<Private> private_;

      void update_select_expressions();
      void execute_query();
      void rebuild_join_map();
      void build_join(std::string from_alias, const IRecordType* from_type, const IAssociation& assoc, CloningPtr<RelationProjector> projector, ast::Join::Type type);
      std::string alias_for(const IRecordType*) const;
      RelationProjector* primary_projector() const;
    };
  }

  template <typename Primary, typename... Relations>
  struct Projection<Primary, Joins<Relations...>> : detail::ProjectionBase {
    // Utility typedefs:
    using Base = detail::ProjectionBase;
    using Self = Projection<Primary, Joins<Relations...>>;
    template <typename AddedAssociation>
    using SelfJoining = Projection<Primary, Joins<AddedAssociation, Relations...>>;
    using TypesInProjection = meta::TypeList<Primary, Relations...>;

    explicit Projection(Context& ctx)
    : Base(ctx, make_cloning_ptr(new detail::RelationProjectorFor<Primary>()))
    {}

    Projection(Context& ctx, std::string primary_alias)
    : Base(ctx, make_cloning_ptr(new detail::RelationProjectorFor<Primary>(std::move(primary_alias))))
    {}

    Projection(const Self& other)
    : Base(other)
    {}

    Self& operator=(const Self& other) {
      Base::operator=(other);
      return *this;
    }

    Self& operator=(Self&& other) {
      Base::operator=(std::move(other));
      return *this;
    }

    // Type-unsafe operations that always compile, but don't give you any compile-time checks:
    Self where(SQL sql) && { return replace_p(std::move(projection_).where(std::move(sql))); }
    Self order(SQL sql) && { return replace_p(std::move(projection_).order(std::move(sql))); }
    Self reverse_order() && { return replace_p(std::move(projection_).reverse_order()); }
    Self limit(size_t n) && { return replace_p(std::move(projection_).limit(n)); }
    Self offset(size_t n) && { return replace_p(std::move(projection_).offset(n)); }

    // Const versions that return a copy:
    Self where(SQL sql) const& { return copy().where(std::move(sql)); }
    Self order(SQL sql) const& { return copy().order(std::move(sql)); }
    Self reverse_order() const& { return copy().reverse_order(); }
    Self limit(size_t n) const& { return copy().limit(n); }
    Self offset(size_t n) const& { return copy().offset(n); }

    // Joins:
    Self inner_join(std::string relation, std::string as, relational_algebra::Condition on) && { return replace_p(std::move(projection_).inner_join(std::move(relation), std::move(as), std::move(on))); }
    Self left_join(std::string relation, std::string as, relational_algebra::Condition on) && { return replace_p(std::move(projection_).left_join(std::move(relation), std::move(as), std::move(on))); }
    Self right_join(std::string relation, std::string as, relational_algebra::Condition on) && { return replace_p(std::move(projection_).right_join(std::move(relation), std::move(as), std::move(on))); }
    Self inner_join(std::string relation, std::string as, relational_algebra::Condition on) const& { return copy().inner_join(std::move(relation), std::move(as), std::move(on)); }
    Self left_join(std::string relation, std::string as, relational_algebra::Condition on) const& { return copy().left_join(std::move(relation), std::move(as), std::move(on)); }
    Self right_join(std::string relation, std::string as, relational_algebra::Condition on) const& { return copy().right_join(std::move(relation), std::move(as), std::move(on)); }

    // Templated versions:
    // Fetching results:
    void each(std::function<void(Primary&)> callback) {
      each([&](RecordPtr<Primary>& ptr) {
        callback(*ptr);
      });
    }
    void each(std::function<void(RecordPtr<Primary>&)> callback) {
      execute_query();
      size_t num_rows = results_->height();
      for (size_t i = 0; i < num_rows; ++i) {
        auto ptr = project(i);
        callback(ptr);
      }
    }

    std::vector<RecordPtr<Primary>>
    all() {
      execute_query();
      size_t num_rows = results_->height();
      std::vector<RecordPtr<Primary>> records;
      records.reserve(num_rows);
      for (size_t i = 0; i < num_rows; ++i) {
        records.push_back(project(i));
      }
      return std::move(records);
    }

    RecordPtr<Primary>
    first() && {
      auto p = std::move(*this).limit(1);
      auto records = p.all();
      return records.size() ? std::move(records[0]) : RecordPtr<Primary>(nullptr);
    }

    RecordPtr<Primary>
    first() const& {
      auto p = this->limit(1);
      auto records = p.all();
      return records.size() ? std::move(records[0]) : RecordPtr<Primary>(nullptr);
    }

    // Type-safe operations for tables representing T:
    // TODO: Do some type checking on Condition, to check that it only references tables mentioned in the TypeList.
    Self where(relational_algebra::Condition cond) && { return replace_p(std::move(projection_).where(std::move(cond))); }
    Self where(relational_algebra::Condition cond) const& { return copy().where(std::move(cond)); }

    template <typename Table, typename T>
    Self order(Column<Table, T> col) && {
      static_assert(meta::Contains<Table, TypesInProjection>::Value, "The specified order column belongs to a type that isn't part of this projection.");
      return replace_p(std::move(projection_).order({col.value()}));
    }

    template <typename Table, typename T>
    Self order(Column<Table, T> column) const& {
      return copy().order(std::move(column));
    }

    template <typename Table, typename T>
    Self order(T Table::*col) && {
      return std::move(*this).order(Column<Table, T>(col));
    }

    template <typename Table, typename T>
    Self order(T Table::*col) const& {
      return copy().order(col);
    }

    // Association Joins:
    template <typename Owner, typename Association>
    SelfJoining<Association> inner_join(BelongsTo<Association> Owner::*assoc) && {
      return add_join(assoc, ast::Join::Type::Inner);
    }
    template <typename Owner, typename Association>
    SelfJoining<Association> left_outer_join(BelongsTo<Association> Owner::*assoc) && {
      return add_join(assoc, ast::Join::Type::LeftOuter);
    }
    template <typename Owner, typename Association>
    SelfJoining<Association> right_outer_join(BelongsTo<Association> Owner::*assoc) && {
      return add_join(assoc, ast::Join::Type::RightOuter);
    }

    template <typename Owner, typename Association>
    SelfJoining<Association> inner_join(BelongsTo<Association> Owner::*assoc) const& {
      return copy().inner_join(assoc);
    }
    template <typename Owner, typename Association>
    SelfJoining<Association> left_outer_join(BelongsTo<Association> Owner::*assoc) const& {
      return copy().left_outer_join(assoc);
    }
    template <typename Owner, typename Association>
    SelfJoining<Association> right_outer_join(BelongsTo<Association> Owner::*assoc) const& {
      return copy().right_outer_join(assoc);
    }

    template <typename Owner, typename Association>
    SelfJoining<Association> inner_join(BelongsTo<Association> Owner::*assoc, std::string with_alias) && {
      return add_join(assoc, std::move(with_alias), ast::Join::Type::Inner);
    }
    template <typename Owner, typename Association>
    SelfJoining<Association> left_outer_join(BelongsTo<Association> Owner::*assoc, std::string with_alias) && {
      return add_join(assoc, std::move(with_alias), ast::Join::Type::LeftOuter);
    }
    template <typename Owner, typename Association>
    SelfJoining<Association> right_outer_join(BelongsTo<Association> Owner::*assoc, std::string with_alias) && {
      return add_join(assoc, std::move(with_alias), ast::Join::Type::RightOuter);
    }

    template <typename Owner, typename Association>
    SelfJoining<Association> inner_join(BelongsTo<Association> Owner::*assoc, std::string with_alias) const& {
      return copy().inner_join(assoc, std::move(with_alias));
    }
    template <typename Owner, typename Association>
    SelfJoining<Association> left_outer_join(BelongsTo<Association> Owner::*assoc, std::string with_alias) const& {
      return copy().left_outer_join(assoc, std::move(with_alias));
    }
    template <typename Owner, typename Association>
    SelfJoining<Association> right_outer_join(BelongsTo<Association> Owner::*assoc, std::string with_alias) const& {
      return copy().right_outer_join(assoc, std::move(with_alias));
    }

    template <typename Owner, typename Association>
    SelfJoining<Association> inner_join(std::string from_alias, BelongsTo<Association> Owner::*assoc) && {
      return add_join(std::move(from_alias), assoc, ast::Join::Type::Inner);
    }
    template <typename Owner, typename Association>
    SelfJoining<Association> left_outer_join(std::string from_alias, BelongsTo<Association> Owner::*assoc) && {
      return add_join(std::move(from_alias), assoc, ast::Join::Type::LeftOuter);
    }
    template <typename Owner, typename Association>
    SelfJoining<Association> right_outer_join(std::string from_alias, BelongsTo<Association> Owner::*assoc) && {
      return add_join(std::move(from_alias), assoc, ast::Join::Type::RightOuter);
    }

    template <typename Owner, typename Association>
    SelfJoining<Association> inner_join(std::string from_alias, BelongsTo<Association> Owner::*assoc) const& {
      return copy().inner_join(std::move(from_alias), assoc);
    }
    template <typename Owner, typename Association>
    SelfJoining<Association> left_outer_join(std::string from_alias, BelongsTo<Association> Owner::*assoc) const& {
      return copy().left_outer_join(std::move(from_alias), assoc);
    }
    template <typename Owner, typename Association>
    SelfJoining<Association> right_outer_join(std::string from_alias, BelongsTo<Association> Owner::*assoc) const& {
      return copy().right_outer_join(std::move(from_alias), assoc);
    }

    template <typename Owner, typename Association>
    SelfJoining<Association> inner_join(std::string from_alias, BelongsTo<Association> Owner::*assoc, std::string with_alias) && {
      return add_join(std::move(from_alias), assoc, std::move(with_alias), ast::Join::Type::Inner);
    }
    template <typename Owner, typename Association>
    SelfJoining<Association> left_outer_join(std::string from_alias, BelongsTo<Association> Owner::*assoc, std::string with_alias) && {
      return add_join(std::move(from_alias), assoc, std::move(with_alias), ast::Join::Type::LeftOuter);
    }
    template <typename Owner, typename Association>
    SelfJoining<Association> right_outer_join(std::string from_alias, BelongsTo<Association> Owner::*assoc, std::string with_alias) && {
      return add_join(std::move(from_alias), assoc, std::move(with_alias), ast::Join::Type::RightOuter);
    }

    template <typename Owner, typename Association>
    SelfJoining<Association> inner_join(std::string from_alias, BelongsTo<Association> Owner::*assoc, std::string with_alias) const& {
      return copy().inner_join(std::move(from_alias), assoc, std::move(with_alias));
    }
    template <typename Owner, typename Association>
    SelfJoining<Association> left_outer_join(std::string from_alias, BelongsTo<Association> Owner::*assoc, std::string with_alias) const& {
      return copy().left_outer_join(std::move(from_alias), assoc, std::move(with_alias));
    }
    template <typename Owner, typename Association>
    SelfJoining<Association> right_outer_join(std::string from_alias, BelongsTo<Association> Owner::*assoc, std::string with_alias) const& {
      return copy().right_outer_join(std::move(from_alias), assoc, std::move(with_alias));
    }

  private:
    template <typename, typename> friend struct Projection;

    // Replace-projection constructor, used by all rvalue member functions through replace_p.
    template <typename... Relations0>
    Projection(Projection<Primary, Joins<Relations0...>> && other, relational_algebra::Projection new_projection)
    : Base(std::move(other), std::move(new_projection))
    {}

    Self replace_p(relational_algebra::Projection p) {
      return Self{std::move(*this), std::move(p)};
    }

    Self copy() const { return *this; }

    template <typename Owner, typename Association>
    SelfJoining<Association>
    add_join(BelongsTo<Association> Owner::*assoc, ast::Join::Type type) {
      static_assert(!meta::Contains<Association, TypesInProjection>::Value, "Cannot add a type-safe join without an alias for a relation that's already included in this projection. Provide an alias for this join.");
      return add_join(assoc, get_type<Association>()->relation(), type);
    }

    template <typename Owner, typename Association>
    SelfJoining<Association>
    add_join(BelongsTo<Association> Owner::*assoc, std::string to_alias, ast::Join::Type type) {
      return add_join(alias_for(get_type<Owner>()), assoc, std::move(to_alias), type);
    }

    template <typename Owner, typename Association>
    SelfJoining<Association>
    add_join(std::string from_alias, BelongsTo<Association> Owner::*assoc, std::string to_alias, ast::Join::Type type) {
      static_assert(meta::Contains<Owner, TypesInProjection>::Value, "Cannot add type-safe join from a field of a type that isn't already part of this projection.");

      auto source_type = get_type<Owner>();
      auto target_type = get_type<Association>();
      auto association = source_type->find_singular_association_by_member_pointer(assoc);

      auto new_projection = SelfJoining<Association>{std::move(*this), std::move(projection_)};
      auto new_projector = make_cloning_ptr(new detail::RelationProjectorFor<Association>(to_alias));
      new_projection.build_join(std::move(from_alias), source_type, *association, std::move(new_projector), type);
      return std::move(new_projection);
    }

    RecordPtr<Primary> project(size_t row) {
      assert(results_ != nullptr);
      // It's safe to static cast because we know what ProjectionBase looks like internally.
      auto p = static_cast<detail::RelationProjectorFor<Primary>*>(primary_projector());
      return p->project(context_, *results_, row);
    }

    template <class, class> friend struct Projection;
  };

  template <typename T>
  Projection<T> from(Context& ctx) {
    return Projection<T>(ctx);
  }

  template <typename T>
  Projection<T> from(Context& ctx, std::string t0_alias) {
    return Projection<T>(ctx, std::move(t0_alias));
  }

  template <typename T>
  RecordPtr<T> find(Context& ctx, PrimaryKey key) {
    return from<T>(ctx).where(column(&T::id) == key).first();
  }
}

#endif // PERSISTENCE_PROJECTION_HPP_INCLUDED

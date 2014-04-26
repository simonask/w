#pragma once
#ifndef PERSISTENCE_PROJECTION_HPP_INCLUDED
#define PERSISTENCE_PROJECTION_HPP_INCLUDED

#include <persistence/result_set.hpp>
#include <persistence/ast.hpp>
#include <persistence/relational_algebra.hpp>
#include <wayward/support/maybe.hpp>
#include <wayward/support/format.hpp>
#include <persistence/column_traits.hpp>
#include <persistence/column_abilities.hpp>
#include <persistence/record_type.hpp>
#include <persistence/connection.hpp>
#include <persistence/connection_provider.hpp>
#include <persistence/belongs_to.hpp>
#include <persistence/context.hpp>
#include <persistence/type_list.hpp>

#include <wayward/support/error.hpp>

#include <functional>
#include <cassert>

namespace persistence {
  using relational_algebra::sql;
  using relational_algebra::SQL;
  using wayward::Maybe;
  using wayward::Nothing;
  using wayward::CloningPtr;

  struct UnregisteredPropertyError : wayward::Error {
    UnregisteredPropertyError(std::string type_name)
    : wayward::Error(wayward::format("Attempted to use unregistered property on type {0}. Use property(member, column) in the PERSISTENCE block for the type to register the property.", type_name))
    {}
  };

  template <typename... Relations> struct Joins;

  template <typename T, typename Jx = Joins<>> struct Projection;

  template <typename Type, typename ColumnType>
  struct Column : ColumnAbilities<Column<Type, ColumnType>, ColumnType> {
    Column(ColumnType Type::*member) {
      const RecordType<Type>* t = get_type<Type>();
      Maybe<std::string> column = t->find_column_by_member_pointer(member);
      if (column) {
        sql = relational_algebra::column(t->relation(), std::move(*column));
      } else {
        throw UnregisteredPropertyError(t->name());
      }
    }

    explicit Column(std::string column_name) {
      const RecordType<Type>* t = get_type<Type>();
      auto property = t->find_property_by_column_name(column_name);
      if (property == nullptr) {
        // TODO: Issue a warning?
      }
      sql = relational_algebra::column(t->relation(), std::move(column_name));
    }

    Column(std::string relation_alias, std::string column_name) {
      const RecordType<Type>* t = get_type<Type>();
      auto property = t->find_property_by_column_name(column_name);
      if (property == nullptr) {
        // TODO: Issue a warning?
      }
      sql = relational_algebra::column(std::move(relation_alias), std::move(column_name));
    }

    relational_algebra::Value sql;
  };

  template <typename Type, typename ColumnType>
  Column<Type, ColumnType> column(ColumnType Type::*member) {
    return Column<Type, ColumnType>(member);
  }
  using relational_algebra::column;

  struct AssociationTypeMismatchError : wayward::Error {
    AssociationTypeMismatchError(std::string msg) : Error(std::move(msg)) {}
  };

  struct RelationProjector : wayward::ICloneable {
    RelationProjector(std::string relation_alias) : relation_alias_(std::move(relation_alias)) {}
    virtual void project_and_populate_association(Context&, ISingularAssociationField&, const IResultSet& result_set, size_t row) = 0;
    virtual void rebuild_column_aliases() {}

    const std::string& relation_alias() const { return relation_alias_; }
    void set_relation_alias(std::string new_relation_alias) {
      relation_alias_ = std::move(new_relation_alias);
      rebuild_column_aliases();
    }
  private:
    std::string relation_alias_;
  };

  template <typename T>
  struct RelationProjectorFor : wayward::Cloneable<RelationProjectorFor<T>, RelationProjector> {
    RelationProjectorFor(std::string relation_alias) : wayward::Cloneable<RelationProjectorFor<T>, RelationProjector>(std::move(relation_alias)) {}

    std::map<const ISingularAssociationFrom<T>*, CloningPtr<RelationProjector>> sub_projectors_;
    std::vector<std::pair<IPropertyOf<T>*, std::string>> column_aliases_;

    void rebuild_column_aliases() final {
      if (column_aliases_.size()) {
        column_aliases_.clear();
      }
      auto record_type = get_type<T>();
      for (auto& property: record_type->properties_) {
        auto alias = wayward::format("{0}_{1}", this->relation_alias(), property->column());
        column_aliases_.push_back(std::make_pair(property.get(), std::move(alias)));
      }
    }

    RecordPtr<T> project(Context& ctx, const IResultSet& result_set, size_t row) {
      rebuild_column_aliases();

      auto record = ctx.create<T>();

      // Populate fields
      for (auto& pair: column_aliases_) {
        auto property = pair.first;
        auto& alias = pair.second;
        property->set(*record, result_set, row, alias);
      }

      // Populate child associations
      for (auto& pair: sub_projectors_) {
        ISingularAssociationField* real_association = pair.first->get_field(*record);
        pair.second->project_and_populate_association(ctx, *real_association, result_set, row);
      }

      return std::move(record);
    }

    void project_and_populate_association(Context& ctx, ISingularAssociationField& association, const IResultSet& result_set, size_t row) {
      auto ptr = project(ctx, result_set, row);
      auto typed_association = dynamic_cast<ISingularAssociationFieldTo<T>*>(&association);
      if (typed_association == nullptr) {
        throw AssociationTypeMismatchError(wayward::format("Could not populate association expecting type {0} with object of type {1}.", association.foreign_type().name(), get_type<T>()->name()));
      }
      typed_association->populate(std::move(ptr));
    }
  };

  template <typename Primary, typename... Relations>
  struct Projection<Primary, Joins<Relations...>> {
    // Utility typedefs:
    using Self = Projection<Primary, Joins<Relations...>>;
    template <typename AddedAssociation>
    using SelfJoining = Projection<Primary, Joins<AddedAssociation, Relations...>>;
    using TypesInProjection = TypeList<Primary, Relations...>;

    // We need to define some constructors, because we're holding a unique_ptr:
    explicit Projection(Context& ctx)
    : context_(ctx)
    {
      auto relation = get_type<Primary>()->relation();
      q_.projector_ = make_cloning_ptr(new RelationProjectorFor<Primary>(relation));
      p_.query->relation = relation;
      update_primary_entry_point();
    }

    Projection(const Self& other)
     : context_(other.context_)
     , materialized_(nullptr)
     , p_(other.p_)
     , q_(other.q_)
     {
      update_primary_entry_point();
     }

    Projection(Self&& other)
    : context_(other.context_)
    , materialized_(std::move(other.materialized_))
    , p_(std::move(other.p_))
    , q_(std::move(other.q_))
    {}

    Self& operator=(const Self& other) {
      q_ = other.q_;
      p_ = other.p_;
      materialized_ = nullptr; // reset because the query has changed.
      update_primary_entry_point();
      return *this;
    }

    Self& operator=(Self&& other) {
      materialized_ = std::move(other.materialized_);
      p_ = std::move(other.p_);
      q_ = std::move(other.q_);
      return *this;
    }

    // Common operations:
    std::string to_sql() {
      update_select_expressions();
      auto conn = current_connection_provider().acquire_connection_for_data_store(get_type<Primary>()->data_store());
      return conn.to_sql(*p_.query);
    }

    size_t count() const;
    void each(std::function<void(std::string)> callback); // TODO: TypedRow type.

    // Type-unsafe operations that always compile, but don't give you any compile-time checks:
    Self where(SQL sql) && { return replace_p(std::move(p_).where(std::move(sql))); }
    Self order(SQL sql) && { return replace_p(std::move(p_).order(std::move(sql))); }
    Self reverse_order() && { return replace_p(std::move(p_).reverse_order()); }
    Self limit(size_t n) && { return replace_p(std::move(p_).limit(n)); }
    Self offset(size_t n) && { return replace_p(std::move(p_).offset(n)); }

    // Const versions that return a copy:
    Self where(SQL sql) const& { return copy().where(std::move(sql)); }
    Self order(SQL sql) const& { return copy().order(std::move(sql)); }
    Self reverse_order() const& { return copy().reverse_order(); }
    Self limit(size_t n) const& { return copy().limit(n); }
    Self offset(size_t n) const& { return copy().offset(n); }

    // Joins:
    Self inner_join(std::string relation, std::string as, relational_algebra::Condition on) && { return replace_p(std::move(p_).inner_join(std::move(relation), std::move(as), std::move(on))); }
    Self left_join(std::string relation, std::string as, relational_algebra::Condition on) && { return replace_p(std::move(p_).left_join(std::move(relation), std::move(as), std::move(on))); }
    Self right_join(std::string relation, std::string as, relational_algebra::Condition on) && { return replace_p(std::move(p_).right_join(std::move(relation), std::move(as), std::move(on))); }
    Self inner_join(std::string relation, std::string as, relational_algebra::Condition on) const& { return copy().inner_join(std::move(relation), std::move(as), std::move(on)); }
    Self left_join(std::string relation, std::string as, relational_algebra::Condition on) const& { return copy().left_join(std::move(relation), std::move(as), std::move(on)); }
    Self right_join(std::string relation, std::string as, relational_algebra::Condition on) const& { return copy().right_join(std::move(relation), std::move(as), std::move(on)); }

    // Templated versions:
    // Fetching results:
    void each(std::function<void(Primary&)> callback) {
      materialize();
      size_t num_rows = materialized_->height();
      for (size_t i = 0; i < num_rows; ++i) {
        auto ptr = project(i);
        callback(*ptr);
      }
    }

    // Type-safe operations for tables representing T:
    // TODO: Do some type checking on Condition, to check that it only references tables mentioned in the TypeList.
    Self where(relational_algebra::Condition cond) && { return replace_p(std::move(p_).where(std::move(cond))); }
    Self where(relational_algebra::Condition cond) const& { return copy().where(std::move(cond)); }

    template <typename Table, typename T>
    Self order(Column<Table, T> column) && {
      static_assert(Contains<Table, TypesInProjection>::Value, "The specified order column belongs to a type that isn't part of this projection.");

    }

    template <typename T>
    Self order(Column<Primary, T> column) const& {
      return copy().order(std::move(column));
    }

    // Association Joins:
    template <typename Owner, typename Association>
    SelfJoining<Association> inner_join(BelongsTo<Association> Owner::*assoc) && {
      return inner_join(assoc, get_type<Association>()->relation());
    }
    template <typename Owner, typename Association>
    SelfJoining<Association> left_outer_join(BelongsTo<Association> Owner::*assoc) && {
      return left_outer_join(assoc, get_type<Association>()->relation());
    }
    template <typename Owner, typename Association>
    SelfJoining<Association> right_outer_join(BelongsTo<Association> Owner::*assoc) && {
      return right_outer_join(assoc, get_type<Association>()->relation());
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

  private:
    template <typename, typename> friend struct Projection;

    // Replace-projection constructor, used by all rvalue member functions through replace_p.
    template <typename... Relations0>
    Projection(Projection<Primary, Joins<Relations0...>> && other, relational_algebra::Projection new_projection)
    : context_(other.context_)
    , materialized_(nullptr) // replace data because the query has changed.
    , p_(std::move(new_projection))
    {
      q_.entry_points_ = std::move(other.q_.entry_points_);
      q_.projector_ = std::move(other.q_.projector_);
    }

    Self replace_p(relational_algebra::Projection p) {
      return Self{std::move(*this), std::move(p)};
    }

    Self copy() const { return *this; }

    template <typename Owner, typename Association>
    SelfJoining<Association>
    add_join(BelongsTo<Association> Owner::*assoc, std::string with_alias, ast::Join::Type type) {
      static_assert(Contains<Owner, TypesInProjection>::Value, "Cannot add type-safe join from a field of a type that isn't already part of this projection.");
      static_assert(!Contains<Association, TypesInProjection>::Value, "NIY: Cannot add type-safe join for this type, because it is already part of the joins for this projection. Use a type-unsafe named join instead.");

      auto source_type = get_type<Owner>();
      auto target_type = get_type<Association>();
      auto association = source_type->find_singular_association_by_member_pointer(assoc);

      // Build the entry point and hook it up in all the right places:
      auto entry_point = q_.entry_points_[source_type];
      auto typed_entry_point = dynamic_cast<RelationProjectorFor<Owner>*>(entry_point);
      assert(typed_entry_point != nullptr); // The type made it into the template parameter list, but its type info pointer didn't make it to the entry points list.
      auto new_entry_point = new RelationProjectorFor<Association>(with_alias);
      typed_entry_point->sub_projectors_[association] = make_cloning_ptr(new_entry_point);
      q_.entry_points_[target_type] = new_entry_point;

      // Prepare information for the JOIN:
      auto& source_alias = entry_point->relation_alias();
      std::string relation = target_type->relation();

      // Build the condition with the relational algebra DSL:
      auto cond = (column(with_alias, target_type->primary_key()->column()) == column(source_alias, association->foreign_key()));

      // Combine it all to a join:
      switch (type) {
        case ast::Join::Inner:      return SelfJoining<Association>(std::move(*this), std::move(p_).inner_join(std::move(relation), std::move(with_alias), std::move(cond)));
        case ast::Join::LeftOuter:  return SelfJoining<Association>(std::move(*this), std::move(p_).left_join(std::move(relation), std::move(with_alias), std::move(cond)));
        case ast::Join::RightOuter: return SelfJoining<Association>(std::move(*this), std::move(p_).right_join(std::move(relation), std::move(with_alias), std::move(cond)));
        default: assert(false); // NIY
      }
    }

    void materialize() {
      if (materialized_ == nullptr) {
        update_select_expressions();
        auto conn = current_connection_provider().acquire_connection_for_data_store(get_type<Primary>()->data_store());
        materialized_ = conn.execute(*p_.query);
      }
    }

    RecordPtr<Primary> project(size_t row) {
      assert(materialized_ != nullptr);
      return q_.projector_->project(context_, *materialized_, row);
    }

    void update_primary_entry_point() {
      q_.entry_points_[get_type<Primary>()] = q_.projector_.get();
    }

    void update_select_expressions() {
      std::vector<relational_algebra::SelectAlias> selects;

      for (auto& pair: q_.entry_points_) {
        auto record_type = pair.first;
        auto projector = pair.second;
        auto& rel = projector->relation_alias();
        for (size_t i = 0; i < record_type->num_properties(); ++i) {
          auto col = record_type->property_at(i).column();
          auto alias = wayward::format("{0}_{1}", rel, col);
          selects.emplace_back(relational_algebra::column(rel, col), alias);
        }
      }

      p_ = std::move(p_).select(std::move(selects));
    }

    // Things that can't be copied:
    Context& context_;
    std::unique_ptr<IResultSet> materialized_;

    // Things that can:
    struct QueryInfo {
      std::map<const IRecordType*, RelationProjector*> entry_points_;
      CloningPtr<RelationProjectorFor<Primary>> projector_; // This lives on the heap to avoid having to fix up the primary entry point every time we move.
    };

    relational_algebra::Projection p_;
    QueryInfo q_;
  };

  template <typename T>
  Projection<T> from(Context& ctx) {
    return Projection<T>(ctx);
  }

  template <typename T>
  Projection<T> from(Context& ctx, std::string t0_alias);
}

#endif // PERSISTENCE_PROJECTION_HPP_INCLUDED

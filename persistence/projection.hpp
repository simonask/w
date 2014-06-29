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

  struct UnregisteredPropertyError : wayward::Error {
    UnregisteredPropertyError(std::string type_name)
    : wayward::Error(wayward::format("Attempted to use unregistered property on type {0}. Use property(member, column) in the PERSISTENCE block for the type to register the property.", type_name))
    {}
  };

  template <typename... Relations> struct Joins;

  template <typename T, typename Jx = Joins<>> struct Projection;

  template <typename Type, typename ColumnType>
  struct Column : ColumnAbilities<Column<Type, ColumnType>, ColumnType> {
    std::string column_name;
    Maybe<std::string> explicit_alias;

    Column(ColumnType Type::*member) {
      const RecordType<Type>* t = get_type<Type>();
      Maybe<std::string> column = t->find_column_by_member_pointer(member);
      if (column) {
        column_name = *column;
      } else {
        throw UnregisteredPropertyError(t->name());
      }
    }

    Column(std::string relation_alias, ColumnType Type::*member) : explicit_alias(std::move(relation_alias)) {
      const RecordType<Type>* t = get_type<Type>();
      Maybe<std::string> column = t->find_column_by_member_pointer(member);
      if (column) {
        column_name = *column;
      } else {
        throw UnregisteredPropertyError(t->name());
      }
    }

    Column(std::string column_name) : column_name(std::move(column_name)) {}
    Column(std::string relation_alias, std::string column_name) : column_name(std::move(column_name)), explicit_alias(std::move(relation_alias)) {}

    relational_algebra::Value value() const& {
      if (explicit_alias) {
        return relational_algebra::column(*explicit_alias, column_name);
      } else {
        return relational_algebra::column(reinterpret_cast<ast::SymbolicRelation>(get_type<Type>()), column_name);
      }
    }

    relational_algebra::Value value() && {
      if (explicit_alias) {
        return relational_algebra::column(std::move(*explicit_alias), std::move(column_name));
      } else {
        return relational_algebra::column(reinterpret_cast<ast::SymbolicRelation>(get_type<Type>()), std::move(column_name));
      }
    }
  };

  template <typename Type, typename ColumnType>
  Column<Type, ColumnType> column(ColumnType Type::*member) {
    return Column<Type, ColumnType>(member);
  }
  template <typename Type, typename ColumnType>
  Column<Type, ColumnType> column(std::string alias, ColumnType Type::*member) {
    return Column<Type, ColumnType>(std::move(alias), member);
  }
  template <typename Type, typename ColumnType>
  Column<Type, ColumnType> column(std::string column) {
    return Column<Type, ColumnType>(std::move(column));
  }
  template <typename Type, typename ColumnType>
  Column<Type, ColumnType> column(std::string alias, std::string column) {
    return Column<Type, ColumnType>(std::move(alias), std::move(column));
  }

  struct AssociationTypeMismatchError : wayward::Error {
    AssociationTypeMismatchError(std::string msg) : Error(std::move(msg)) {}
  };

  struct RelationProjector : wayward::ICloneable {
    RelationProjector(std::string relation_alias) : relation_alias_(std::move(relation_alias)) {}
    virtual void project_and_populate_association(Context&, IAssociationAnchor&, const IResultSet& result_set, size_t row) = 0;

    virtual void append_selects(std::vector<relational_algebra::SelectAlias>& out_selects) const = 0;
    virtual void rebuild_joins(std::map<std::string, RelationProjector*>& out_joins) = 0;

    const std::string& relation_alias() const { return relation_alias_; }
  private:
    std::string relation_alias_;
  };

  template <typename T>
  struct RelationProjectorFor : wayward::Cloneable<RelationProjectorFor<T>, RelationProjector> {
    std::map<const IAssociationFrom<T>*, CloningPtr<RelationProjector>> sub_projectors_;
    std::vector<std::pair<IPropertyOf<T>*, std::string>> column_aliases_;

    RelationProjectorFor(std::string relation_alias) : wayward::Cloneable<RelationProjectorFor<T>, RelationProjector>(std::move(relation_alias))
    {
      auto record_type = get_type<T>();
      for (auto& property: record_type->properties_) {
        auto alias = wayward::format("{0}_{1}", this->relation_alias(), property->column());
        column_aliases_.push_back(std::make_pair(property.get(), std::move(alias)));
      }
    }

    void rebuild_joins(std::map<std::string, RelationProjector*>& out_joins) final {
      out_joins[this->relation_alias()] = static_cast<RelationProjector*>(this);
      for (auto& pair: sub_projectors_) {
        pair.second->rebuild_joins(out_joins);
      }
    }

    void append_selects(std::vector<relational_algebra::SelectAlias>& out_selects) const final {
      for (auto& pair: column_aliases_) {
        out_selects.emplace_back(relational_algebra::column(this->relation_alias(), pair.first->column()), pair.second);
      }
      for (auto& pair: sub_projectors_) {
        pair.second->append_selects(out_selects);
      }
    }

    RecordPtr<T> project(Context& ctx, const IResultSet& result_set, size_t row) {
      auto record = ctx.create<T>();

      // Populate fields
      for (auto& pair: column_aliases_) {
        auto property = pair.first;
        auto& alias = pair.second;
        Maybe<std::string> col_value = result_set.get(row, alias);
        wayward::data_franca::Adapter<Maybe<std::string>> reader { col_value, wayward::data_franca::Options::None };
        //property->deserialize(*record, reader); // TODO!
      }

      // Populate child associations
      for (auto& pair: sub_projectors_) {
        auto real_association = pair.first->get_anchor(*record);
        pair.second->project_and_populate_association(ctx, *real_association, result_set, row);
      }

      return std::move(record);
    }

    void project_and_populate_association(Context& ctx, IAssociationAnchor& association, const IResultSet& result_set, size_t row) {
      auto ptr = project(ctx, result_set, row);
      auto typed_association = dynamic_cast<ISingularAssociationAnchor<T>*>(&association);
      if (typed_association == nullptr) {
        throw AssociationTypeMismatchError(wayward::format("Could not populate association expecting type {0} with object of type {1}.", association.association()->foreign_type()->name(), get_type<T>()->name()));
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
    using TypesInProjection = meta::TypeList<Primary, Relations...>;

    // We need to define some constructors, because we're holding a unique_ptr:
    explicit Projection(Context& ctx)
    : context_(ctx)
    {
      auto relation = get_type<Primary>()->relation();
      q_.projector_ = make_cloning_ptr(new RelationProjectorFor<Primary>(relation));
      p_.query->relation = relation;
      q_.joins_[relation] = q_.projector_.get();
      q_.first_relations_[get_type<Primary>()] = relation;
    }

    Projection(Context& ctx, std::string t0_alias)
    : context_(ctx)
    {
      q_.projector_ = make_cloning_ptr(new RelationProjectorFor<Primary>(t0_alias));
      p_.query->relation = get_type<Primary>()->relation();
      p_.query->relation_alias = t0_alias;
      q_.joins_[t0_alias] = q_.projector_.get();
      q_.first_relations_[get_type<Primary>()] = t0_alias;
    }

    Projection(const Self& other)
     : context_(other.context_)
     , materialized_(nullptr)
     , p_(other.p_)
     , q_(other.q_)
     {
      rebuild_joins();
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
      rebuild_joins();
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
      return conn.to_sql(*p_.query, q_);
    }

    size_t count() const {
      if (materialized_)
        return materialized_->height();

      auto p_copy = p_.select({
        {relational_algebra::aggregate("COUNT", relational_algebra::column(q_.projector_->relation_alias(), get_type<Primary>()->primary_key()->column())),
        "count"}
      });
      auto conn = current_connection_provider().acquire_connection_for_data_store(get_type<Primary>()->data_store());
      auto results = conn.execute(*p_copy.query, q_);
      uint64_t count = 0;
      Maybe<std::string> count_column = results->get(0, "count");
      std::stringstream ss(*count_column);
      ss >> count;
      return count;
    }

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
      each([&](RecordPtr<Primary>& ptr) {
        callback(*ptr);
      });
    }
    void each(std::function<void(RecordPtr<Primary>&)> callback) {
      materialize();
      size_t num_rows = materialized_->height();
      for (size_t i = 0; i < num_rows; ++i) {
        auto ptr = project(i);
        callback(ptr);
      }
    }

    std::vector<RecordPtr<Primary>>
    all() {
      materialize();
      size_t num_rows = materialized_->height();
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
    Self where(relational_algebra::Condition cond) && { return replace_p(std::move(p_).where(std::move(cond))); }
    Self where(relational_algebra::Condition cond) const& { return copy().where(std::move(cond)); }

    template <typename Table, typename T>
    Self order(Column<Table, T> col) && {
      static_assert(meta::Contains<Table, TypesInProjection>::Value, "The specified order column belongs to a type that isn't part of this projection.");
      return replace_p(std::move(p_).order({col.value()}));
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
    : context_(other.context_)
    , materialized_(nullptr) // replace data because the query has changed.
    , p_(std::move(new_projection))
    {
      q_.projector_ = std::move(other.q_.projector_);
      q_.joins_     = std::move(other.q_.joins_);
      q_.first_relations_ = std::move(other.q_.first_relations_);
    }

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
      auto it = q_.first_relations_.find(get_type<Owner>());
      if (it != q_.first_relations_.end()) {
        auto from_alias = it->second;
        return add_join(std::move(from_alias), assoc, std::move(to_alias), type);
      } else {
        assert(false); // Consistency error: first_relations_ has not been properly updated.
      }
    }

    template <typename Owner, typename Association>
    SelfJoining<Association>
    add_join(std::string from_alias, BelongsTo<Association> Owner::*assoc, std::string to_alias, ast::Join::Type type) {
      static_assert(meta::Contains<Owner, TypesInProjection>::Value, "Cannot add type-safe join from a field of a type that isn't already part of this projection.");

      auto source_type = get_type<Owner>();
      auto target_type = get_type<Association>();
      auto association = source_type->find_singular_association_by_member_pointer(assoc);

      // Look up where to hook up the join, and perform some sanity checks on the way:
      auto it = q_.joins_.find(from_alias);
      if (it == q_.joins_.end()) {
        throw AssociationError(wayward::format("Unknown relation '{0}'.", from_alias));
      }
      auto projector = it->second;
      auto typed_projector = dynamic_cast<RelationProjectorFor<Owner>*>(projector);
      if (typed_projector == nullptr) {
        throw AssociationError(wayward::format("The relation alias '{0}' does not describe objects of type '{1}'.", from_alias, source_type->name()));
      }

      // Hook it up in the projector hierarchy:
      auto new_projector = new RelationProjectorFor<Association>(to_alias);
      typed_projector->sub_projectors_[association] = make_cloning_ptr(new_projector);

      // Add it to the list of joins:
      q_.joins_[to_alias] = new_projector;

      // If this is the first join with this relation, update the first_relations list:
      if (q_.first_relations_.find(target_type) == q_.first_relations_.end()) {
        q_.first_relations_[target_type] = to_alias;
      }

      // Prepare information for the JOIN condition:
      std::string relation = target_type->relation();

      // Build the condition with the relational algebra DSL:
      auto lhs = relational_algebra::column(from_alias, association->foreign_key());
      auto rhs = relational_algebra::column(to_alias, target_type->primary_key()->column());
      auto cond = (std::move(lhs) == std::move(rhs));

      // Combine it all to a join:
      switch (type) {
        case ast::Join::Inner:      return SelfJoining<Association>(std::move(*this), std::move(p_).inner_join(std::move(relation), std::move(to_alias), std::move(cond)));
        case ast::Join::LeftOuter:  return SelfJoining<Association>(std::move(*this), std::move(p_).left_join(std::move(relation), std::move(to_alias), std::move(cond)));
        case ast::Join::RightOuter: return SelfJoining<Association>(std::move(*this), std::move(p_).right_join(std::move(relation), std::move(to_alias), std::move(cond)));
        default: assert(false); // NIY
      }
    }

    void rebuild_joins() {
      // This is necessary because we're keeping raw pointers in q_.joins_, so when we're a freshly cloned Projection,
      // we need to rebuild that lookup table.

      q_.joins_.clear();
      q_.projector_->rebuild_joins(q_.joins_);
    }

    void materialize() {
      if (materialized_ == nullptr) {
        update_select_expressions();
        auto conn = current_connection_provider().acquire_connection_for_data_store(get_type<Primary>()->data_store());
        //conn.logger()->log(wayward::Severity::Debug, "p", wayward::format("Load {0}", get_type<Primary>()->name()));
        materialized_ = conn.execute(*p_.query, q_);
      }
    }

    RecordPtr<Primary> project(size_t row) {
      assert(materialized_ != nullptr);
      return q_.projector_->project(context_, *materialized_, row);
    }

    void update_select_expressions() {
      std::vector<relational_algebra::SelectAlias> selects;
      q_.projector_->append_selects(selects);
      p_ = std::move(p_).select(std::move(selects));
    }

    // Things that can't be copied:
    Context& context_;
    std::unique_ptr<IResultSet> materialized_;

    // Things that can:
    struct QueryInfo : relational_algebra::IResolveSymbolicRelation {
      // The root projector. It is kept on the heap to avoid fixing up pointers every time
      // we get moved around.
      CloningPtr<RelationProjectorFor<Primary>> projector_;

      // The map of joins is alias=>projector, and it needs to be rebuilt every time we make a copy of this Projection.
      // Ownership of the RelationProjector pointer is held by the hierarchy of projectors.
      // We keep track of these to find the proper RelationProjector on which to add a join.
      std::map<std::string, RelationProjector*> joins_;

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

    relational_algebra::Projection p_;
    QueryInfo q_;
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

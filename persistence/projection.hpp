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

  template <typename... Tx> struct TypeList {};

  template <typename T, typename TL> struct Contains;
  template <typename T>
  struct Contains<T, TypeList<>> {
    static constexpr bool Value = false;
  };
  template <typename T, typename Head, typename... Rest>
  struct Contains<T, TypeList<Head, Rest...>> {
    static constexpr bool Value = std::is_same<T, Head>::value || Contains<T, TypeList<Rest...>>::Value;
  };

  template <typename T, typename TL, size_t I = 0> struct IndexOf;
  template <typename T, size_t I>
  struct IndexOf<T, TypeList<>, I> {}; // Does not exist.
  template <typename T, size_t I, typename... Rest>
  struct IndexOf<T, TypeList<T, Rest...>, I> {
    static constexpr size_t Value = I;
  };
  template <typename T, size_t I, typename Head, typename... Rest>
  struct IndexOf<T, TypeList<Head, Rest...>, I> {
    static constexpr size_t Value = IndexOf<T, TypeList<Rest...>, I + 1>::Value;
  };

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
    virtual void project_and_populate_association(Context&, ISingularAssociationField&, const IResultSet& result_set, size_t row) const = 0;

    std::string relation_alias_;
  };

  template <typename T>
  struct RelationProjectorFor : wayward::Cloneable<RelationProjectorFor<T>, RelationProjector> {
    std::map<const ISingularAssociationFrom<T>*, CloningPtr<RelationProjector>> sub_projectors_;

    RecordPtr<T> project(Context& ctx, const IResultSet& result_set, size_t row) const {
      auto record = ctx.create<T>();
      for (auto& pair: this->sub_projectors_) {
        ISingularAssociationField* real_association = pair.first->get_field(*record);
        pair.second->project_and_populate_association(ctx, *real_association, result_set, row);
      }
      return std::move(record);
    }

    void project_and_populate_association(Context& ctx, ISingularAssociationField& association, const IResultSet& result_set, size_t row) const {
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
    , projector_()
    {
      relation_aliases_[get_type<Primary>()] = "t0";
    }

    Projection(const Self& other)
     : context_(other.context_)
     , p_(other.p_)
     , materialized_(nullptr)
     , relation_aliases_(other.relation_aliases_)
     , projector_(other.projector_)
     {}

    Projection(Self&& other)
    : context_(other.context_)
    , p_(std::move(other.p_))
    , materialized_(std::move(other.materialized_))
    , relation_aliases_(std::move(other.relation_aliases_))
    , projector_(std::move(other.projector_))
    {}

    Self& operator=(const Self& other) {
      p_ = other.p_;
      materialized_ = nullptr;
      relation_aliases_ = other.relation_aliases_;
      projector_ = other.projector_;
      return *this;
    }

    Self& operator=(Self&& other) {
      p_ = std::move(other.p_);
      materialized_ = std::move(other.materialized_);
      relation_aliases_ = std::move(other.relation_aliases_);
      projector_ = std::move(other.projector_);
      return *this;
    }

    // Common operations:
    std::string to_sql() const;
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
      this->materialize();
      size_t num_rows = this->materialized_->height();
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
      return inner_join(assoc, wayward::format("t{0}", relation_aliases_.size()));
    }
    template <typename Owner, typename Association>
    SelfJoining<Association> left_outer_join(BelongsTo<Association> Owner::*assoc) && {
      return left_outer_join(assoc, wayward::format("t{0}", relation_aliases_.size()));
    }
    template <typename Owner, typename Association>
    SelfJoining<Association> right_outer_join(BelongsTo<Association> Owner::*assoc) && {
      return right_outer_join(assoc, wayward::format("t{0}", relation_aliases_.size()));
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
    , materialized_(std::move(other.materialized_))
    , p_(std::move(new_projection))
    , relation_aliases_(std::move(other.relation_aliases_))
    , projector_(std::move(other.projector_))
    {}

    Self replace_p(relational_algebra::Projection p) { return Self{std::move(*this), std::move(p)}; }
    Self copy() const { return *this; }

    template <typename Owner, typename Association>
    SelfJoining<Association>
    add_join(BelongsTo<Association> Owner::*assoc, std::string with_alias, ast::Join::Type type) {
      static_assert(Contains<Owner, TypesInProjection>::Value, "Cannot add type-safe join from a field of a type that isn't already part of this projection.");
      static_assert(!Contains<Association, TypesInProjection>::Value, "NIY: Cannot add type-safe join for this type, because it is already part of the joins for this projection. Use a type-unsafe named join instead.");

      auto source_type = get_type<Owner>();
      auto target_type = get_type<Association>();
      relation_aliases_[target_type] = with_alias;
      auto source_alias = relation_aliases_[get_type<Owner>()];
      std::string relation = target_type->relation();
      auto association = source_type->find_singular_association_by_member_pointer(assoc);
      auto cond = (column(with_alias, target_type->primary_key()->column()) == column(source_alias, association->foreign_key()));

      switch (type) {
        case ast::Join::Inner:      return SelfJoining<Association>(std::move(*this), std::move(p_).inner_join(std::move(relation), std::move(with_alias), std::move(cond)));
        case ast::Join::LeftOuter:  return SelfJoining<Association>(std::move(*this), std::move(p_).left_join(std::move(relation), std::move(with_alias), std::move(cond)));
        case ast::Join::RightOuter: return SelfJoining<Association>(std::move(*this), std::move(p_).right_join(std::move(relation), std::move(with_alias), std::move(cond)));
        default: assert(false); // NIY
      }
    }

    std::string to_sql() {
      auto conn = current_connection_provider().acquire_connection_for_data_store(get_type<Primary>()->data_store());
      return conn.to_sql(*p_.query);
    }

    void materialize() {
      auto conn = current_connection_provider().acquire_connection_for_data_store(get_type<Primary>()->data_store());
      materialized_ = conn.execute(*p_.query);
    }

    RecordPtr<Primary> project(size_t row) {
      assert(materialized_ != nullptr);
      return projector_.project(context_, *materialized_, row);
    }

    // Things that can't be copied:
    Context& context_;
    std::unique_ptr<IResultSet> materialized_;

    // Things that can:
    relational_algebra::Projection p_;
    std::map<const IRecordType*, std::string> relation_aliases_;
    RelationProjectorFor<Primary> projector_;
  };

  template <typename T>
  Projection<T> from(Context& ctx) {
    return Projection<T>(ctx);
  }

  template <typename T>
  Projection<T> from(Context& ctx, std::string t0_alias);
}

#endif // PERSISTENCE_PROJECTION_HPP_INCLUDED

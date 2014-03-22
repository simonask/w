#pragma once
#ifndef PERSISTENCE_RECORD_TYPE_BUILDER_HPP_INCLUDED
#define PERSISTENCE_RECORD_TYPE_BUILDER_HPP_INCLUDED

#include <persistence/record_type.hpp>
#include <persistence/property.hpp>
#include <persistence/belongs_to.hpp>
#include <persistence/has_many.hpp>
#include <persistence/has_one.hpp>

namespace persistence {
  template <typename RT>
  struct RecordTypeBuilder {
    RecordType<RT>* type_;
    RecordType<RT>* result_() const { return type_; }

    void name(std::string name) { type_->name_ = std::move(name); }
    void relation(std::string table_name) { type_->relation_ = std::move(table_name); }

    template <typename AssociatedType>
    BelongsToAssociation<RT, AssociatedType>&
    belongs_to(BelongsTo<AssociatedType> RT::*member, std::string key_column) {
      auto p = new BelongsToAssociation<RT, AssociatedType> { key_column, member };
      auto prop = new PropertyOf<RT, BelongsTo<AssociatedType>> { member, std::move(key_column) };
      type_->associations_.push_back(std::unique_ptr<IAssociationFrom<RT>>(p));
      type_->properties_.push_back(std::unique_ptr<IPropertyOf<RT>>(prop));
      return *p;
    }

    template <typename AssociatedType>
    HasManyAssociation<RT, AssociatedType>&
    has_many(HasMany<AssociatedType> RT::*member, std::string foreign_key) {
      auto p = new HasManyAssociation<RT, AssociatedType> { std::move(foreign_key), member };
      type_->associations_.push_back(std::unique_ptr<IAssociationFrom<RT>>(p));
      return *p;
    }

    template <typename AssociatedType>
    PropertyOf<RT, HasOne<AssociatedType>>&
    has_one(HasOne<AssociatedType> RT::*member, std::string foreign_key) {
      auto p = new HasOneAssociation<RT, AssociatedType> { std::move(foreign_key), member };
      type_->associations_.push_back(std::unique_ptr<IAssociationFrom<RT>>(p));
      return *p;
    }

    template <typename T>
    PropertyOf<RT, T>&
    property(T RT::*field, std::string column) {
      auto p = new PropertyOf<RT, T>{field, std::move(column)};
      type_->properties_.push_back(std::unique_ptr<IPropertyOf<RT>>(p));

      // TODO: Handle multi-column primary keys?
      if (std::is_same<T, PrimaryKey>::value) {
        type_->primary_key_ = p;
      }

      return *p;
    }
  };
}

#endif // PERSISTENCE_RECORD_TYPE_BUILDER_HPP_INCLUDED


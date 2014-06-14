#pragma once
#ifndef PERSISTENCE_RECORD_TYPE_BUILDER_HPP_INCLUDED
#define PERSISTENCE_RECORD_TYPE_BUILDER_HPP_INCLUDED

#include <persistence/record_type.hpp>
#include <persistence/property.hpp>
#include <persistence/belongs_to.hpp>
#include <persistence/has_many.hpp>
#include <persistence/has_one.hpp>
#include <persistence/record_as_structured_data.hpp>

#include <wayward/support/maybe.hpp>

namespace persistence {
  using wayward::Maybe;
  using wayward::Nothing;

  template <typename RT>
  struct RecordTypeBuilder {
    RecordType<RT>* type_;
    RecordType<RT>* result_() const { return type_; }

    void name(std::string name) { type_->name_ = std::move(name); }
    void relation(std::string table_name) { type_->relation_ = std::move(table_name); }
    void data_store(std::string data_store) { type_->data_store_ = std::move(data_store); }

    template <typename AssociatedType>
    BelongsToAssociation<RT, AssociatedType>&
    belongs_to(BelongsTo<AssociatedType> RT::*member, std::string name, Maybe<std::string> foreign_key = Nothing) {
      std::string fkey;
      if (foreign_key) {
        fkey = *foreign_key;
      } else {
        fkey = name + "_id";
      }

      auto p = new BelongsToAssociation<RT, AssociatedType> { member, name, fkey };
      auto prop = new PropertyOf<RT, BelongsTo<AssociatedType>> { member, fkey };
      type_->associations_.push_back(std::unique_ptr<IAssociationFrom<RT>>(p));
      type_->properties_.push_back(std::unique_ptr<IPropertyOf<RT>>(prop));
      return *p;
    }

    template <typename AssociatedType>
    HasManyAssociation<RT, AssociatedType>&
    has_many(HasMany<AssociatedType> RT::*member, std::string name, std::string foreign_key) {
      auto p = new HasManyAssociation<RT, AssociatedType> { member, name, foreign_key };
      type_->associations_.push_back(std::unique_ptr<IAssociationFrom<RT>>(p));
      return *p;
    }

    template <typename AssociatedType>
    PropertyOf<RT, HasOne<AssociatedType>>&
    has_one(HasOne<AssociatedType> RT::*member, std::string name, std::string foreign_key) {
      auto p = new HasOneAssociation<RT, AssociatedType> { member, name, foreign_key };
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


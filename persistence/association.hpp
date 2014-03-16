#pragma once
#ifndef PERSISTENCE_ASSOCIATIONS_HPP_INCLUDED
#define PERSISTENCE_ASSOCIATIONS_HPP_INCLUDED

#include <persistence/property.hpp>

namespace persistence {
  struct IRecordType;

  struct IAssociation {
    virtual ~IAssociation() {}
    virtual const IRecordType& self_type() const = 0;
    virtual const IRecordType& foreign_type() const = 0;
    virtual std::string foreign_key() const = 0;
    //virtual void populate() const = 0;
  };

  template <typename AssociatedType>
  struct IAssociationTo {
    virtual ~IAssociationTo() {}
  };

  template <typename Owner>
  struct IAssociationFrom : IAssociation {
    virtual ~IAssociationFrom() {}

    virtual void initialize_in_object(Owner& object) const = 0;
  };

  template <typename Owner, typename AssociatedType>
  struct Association : IAssociationFrom<Owner>, IAssociationTo<AssociatedType> {
    explicit Association(std::string key) : key_(std::move(key)) {}
    virtual ~Association() {}

    const IRecordType& self_type() const final { return *get_type<Owner>(); }
    const IRecordType& foreign_type() const final { return *get_type<AssociatedType>(); }
    std::string foreign_key() const override { return key_; }
  protected:
    std::string key_;
  };
}

#endif // PERSISTENCE_ASSOCIATIONS_HPP_INCLUDED

#pragma once
#ifndef PERSISTENCE_ASSOCIATIONS_HPP_INCLUDED
#define PERSISTENCE_ASSOCIATIONS_HPP_INCLUDED

#include <persistence/property.hpp>
#include <persistence/record_ptr.hpp>

namespace persistence {
  struct IRecordType;
  struct Context;

  /// Descriptors for the type reflection system:

  struct IAssociation {
    virtual ~IAssociation() {}
    virtual const IRecordType& self_type() const = 0;
    virtual const IRecordType& foreign_type() const = 0;
    virtual std::string foreign_key() const = 0;
  };

  template <typename AssociatedType>
  struct IAssociationTo {
    virtual ~IAssociationTo() {}
  };

  template <typename AssociatedType>
  struct ISingularAssociationTo : IAssociationTo<AssociatedType> {
    virtual ~ISingularAssociationTo() {}
  };

  template <typename AssociatedType>
  struct IPluralAssociationTo : IAssociationTo<AssociatedType> {
    virtual ~IPluralAssociationTo() {}
  };

  template <typename Owner>
  struct IAssociationFrom : IAssociation {
    virtual ~IAssociationFrom() {}

    virtual void initialize_in_object(Owner& object, Context* context) const = 0;
  };

  struct ISingularAssociationField;
  template <typename Owner>
  struct ISingularAssociationFrom : IAssociationFrom<Owner> {
    virtual ~ISingularAssociationFrom() {}

    virtual ISingularAssociationField* get_field(Owner& object) const = 0;
    virtual const ISingularAssociationField* get_field(const Owner& object) const = 0;
  };

  struct IPluralAssociationField;
  template <typename Owner>
  struct IPluralAssociationFrom : IAssociationFrom<Owner> {
    virtual ~IPluralAssociationFrom() {}

    virtual IPluralAssociationField* get_field(Owner& object) const = 0;
    virtual const IPluralAssociationField* get_field(const Owner& object) const = 0;
  };

  template <typename Owner, typename AssociatedType>
  struct SingularAssociation : ISingularAssociationFrom<Owner>, ISingularAssociationTo<AssociatedType> {
    explicit SingularAssociation(std::string key) : key_(std::move(key)) {}
    virtual ~SingularAssociation() {}

    const IRecordType& self_type() const final { return *get_type<Owner>(); }
    const IRecordType& foreign_type() const final { return *get_type<AssociatedType>(); }
    std::string foreign_key() const override { return key_; }
  protected:
    std::string key_;
  };

  template <typename Owner, typename AssociatedType>
  struct PluralAssociation : IPluralAssociationFrom<Owner>, IPluralAssociationTo<AssociatedType> {
    explicit PluralAssociation(std::string key) : key_(std::move(key)) {}
    virtual ~PluralAssociation() {}

    const IRecordType& self_type() const final { return *get_type<Owner>(); }
    const IRecordType& foreign_type() const final { return *get_type<AssociatedType>(); }
    std::string foreign_key() const override { return key_; }
  protected:
    std::string key_;
  };

  /// Interface for struct members:

  struct ISingularAssociationField {
    virtual ~ISingularAssociationField() {}
    virtual bool is_populated() const = 0;
    virtual bool is_set() const = 0;
    virtual const IRecordType& foreign_type() const = 0;
  };

  template <typename T>
  struct ISingularAssociationFieldTo : ISingularAssociationField {
    using Type = T;

    virtual ~ISingularAssociationFieldTo() {}
    virtual void populate(RecordPtr<T> record) = 0;
    virtual RecordPtr<T> get() = 0;
  };

  struct IPluralAssociationField {
    virtual ~IPluralAssociationField() {}
    virtual bool is_populated() const = 0;
    virtual const IRecordType& foreign_type() const = 0;
  };

  template <typename T>
  struct IPluralAssociationFieldTo : IPluralAssociationField {
    virtual ~IPluralAssociationFieldTo() {}

    // TODO: Use a better RecordCollection type, that doesn't copy the context lifetime sentinel for each record.
    virtual void populate(std::vector<RecordPtr<T>> records) = 0;
  };
}

#endif // PERSISTENCE_ASSOCIATIONS_HPP_INCLUDED

#pragma once
#ifndef PERSISTENCE_RECORD_TYPE_INCLUDED
#define PERSISTENCE_RECORD_TYPE_INCLUDED

#include <persistence/type.hpp>
#include <persistence/property.hpp>
#include <persistence/association.hpp>
#include <wayward/support/maybe.hpp>

#include <vector>
#include <memory>

namespace persistence {
  struct IRecordType : IType {
    virtual std::string relation() const = 0;
    virtual void initialize_associations_in_object(void*) const = 0;

    virtual const IProperty* primary_key() const = 0;

    virtual size_t num_properties() const = 0;
    virtual const IProperty& property_at(size_t idx) const = 0;

    virtual size_t num_associations() const = 0;
    virtual const IAssociation& association_at(size_t idx) const = 0;
  };

  struct RecordTypeBase : IRecordType {
    // IType
    std::string name() const final { return name_; }
    bool is_nullable() const final { return false; }

    // IRecordType
    std::string relation() const final { return relation_; }

    // Internal
    std::string name_;
    std::string relation_;
  };

  template <typename RT>
  struct RecordType : RecordTypeBase {
    // TODO: Constructors, destructors, etc.
    std::vector<std::unique_ptr<IPropertyOf<RT>>> properties_;
    std::vector<std::unique_ptr<IAssociationFrom<RT>>> associations_;
    const IPropertyOf<RT>* primary_key_ = nullptr;

    void initialize_associations_in_object(void* obj) const final {
      RT& object = *reinterpret_cast<RT*>(obj);
      for (auto& p: associations_) {
        p->initialize_in_object(object);
      }
    }

    const IProperty* primary_key() const final { return primary_key_; }

    size_t num_properties() const final { return properties_.size(); }
    const IProperty& property_at(size_t idx) const final { return *properties_.at(idx); }

    size_t num_associations() const final { return associations_.size(); }
    const IAssociation& association_at(size_t idx) const final { return *associations_.at(idx); }

    template <typename M>
    const PropertyOf<RT, M>*
    find_property_by_member_pointer(M RT::*member) const {
      for (auto& prop: properties_) {
        auto p = dynamic_cast<const PropertyOf<RT, M>*>(prop.get());
        if (p) return p;
      }
      return nullptr;
    }

    template <typename M>
    w::Maybe<std::string> find_column_by_member_pointer(M RT::*member) const {
      // TODO: We can optimize this by indexing properties by their types.
      auto p = find_property_by_member_pointer(member);
      if (p) {
        return p->column();
      } else {
        return w::Nothing;
      }
    }
  };

  template <typename T>
  void initialize_associations(T& object) {
    get_type<T>()->initialize_associations_in_object(&object);
  }
}

#endif // PERSISTENCE_RECORD_TYPE_INCLUDED

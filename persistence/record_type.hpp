#pragma once
#ifndef PERSISTENCE_RECORD_TYPE_INCLUDED
#define PERSISTENCE_RECORD_TYPE_INCLUDED

#include <wayward/support/type.hpp>

#include <persistence/property.hpp>
#include <persistence/association.hpp>
#include <wayward/support/maybe.hpp>

#include <vector>
#include <memory>

namespace persistence {
  using wayward::IType;

  struct IRecordType : IType {
    virtual std::string relation() const = 0;
    virtual std::string data_store() const = 0;
    virtual void initialize_associations_in_object(void*, Context*) const = 0;

    virtual const IProperty* find_abstract_property_by_column_name(const std::string& name) const = 0;
    virtual const IAssociation* find_abstract_association_by_name(const std::string& name) const = 0;

    virtual const IProperty* abstract_primary_key() const = 0;

    virtual size_t num_properties() const = 0;
    virtual size_t num_associations() const = 0;
    virtual const IProperty* abstract_property_at(size_t idx) const = 0;
    virtual const IAssociation* abstract_association_at(size_t idx) const = 0;
  };

  template <class Base>
  struct RecordTypeBase : Base {
    // IType
    std::string name() const final { return name_; }
    bool is_nullable() const final { return false; }

    // IRecordType
    std::string relation() const final { return relation_; }
    std::string data_store() const final { return data_store_; }

    // Internal
    std::string name_;
    std::string relation_;
    std::string data_store_ = "default";
  };

  template <class RT>
  struct RecordType : RecordTypeBase<wayward::DataTypeFor<RT, IRecordType>> {
    // TODO: Constructors, destructors, etc.
    std::vector<std::unique_ptr<IPropertyOf<RT>>> properties_;
    std::vector<std::unique_ptr<IAssociationFrom<RT>>> associations_;
    const IPropertyOf<RT>* primary_key_ = nullptr;

    bool has_value(const RT&) const final { return true; }

    void visit(RT& value, wayward::DataVisitor& visitor) const final {
      for (auto& prop: properties_) {
        prop->visit(value, visitor);
      }
    }

    void initialize_associations_in_object(void* obj, Context* ctx) const final {
      RT& object = *reinterpret_cast<RT*>(obj);
      for (auto& p: associations_) {
        p->initialize_in_object(object, ctx);
      }
    }

    // IRecordType interface:
    const IProperty* find_abstract_property_by_column_name(const std::string& name) const final { return find_property_by_column_name(name); }
    const IAssociation* find_abstract_association_by_name(const std::string& name) const final { return find_association_by_name(name); }
    const IProperty* abstract_primary_key() const final { return primary_key(); }
    size_t num_properties() const final { return properties_.size(); }
    size_t num_associations() const final { return associations_.size(); }
    const IProperty* abstract_property_at(size_t idx) const final { return property_at(idx); }
    const IAssociation* abstract_association_at(size_t idx) const final { return association_at(idx); }

    // RecordType interface:
    const IPropertyOf<RT>* primary_key() const { return primary_key_; }
    const IPropertyOf<RT>* property_at(size_t idx) const { return properties_.at(idx).get(); }
    const IAssociationFrom<RT>* association_at(size_t idx) const { return associations_.at(idx).get(); }

    const IPropertyOf<RT>* find_property_by_column_name(const std::string& name) const {
      for (auto& prop: properties_) {
        if (prop->column() == name) {
          return prop.get();
        }
      }
      return nullptr;
    }

    const IAssociationFrom<RT>* find_association_by_name(const std::string& name) const {
      for (auto& assoc: associations_) {
        if (assoc->name() == name) {
          return assoc.get();
        }
      }
      return nullptr;
    }

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
    auto find_singular_association_by_member_pointer(M RT::*member) const -> const SingularAssociationBase<RT, M>* {
      using Assoc = SingularAssociationBase<RT, M>;
      for (auto& assoc: associations_) {
        auto p = dynamic_cast<const Assoc*>(assoc.get());
        if (p && p->member_ptr() == member) return p;
      }
      return nullptr;
    }

    template <typename M>
    wayward::Maybe<std::string> find_column_by_member_pointer(M RT::*member) const {
      // TODO: We can optimize this by indexing properties by their types.
      auto p = find_property_by_member_pointer(member);
      if (p) {
        return p->column();
      } else {
        return wayward::Nothing;
      }
    }
  };

  template <typename T>
  void initialize_associations(T& object, Context& ctx) {
    get_type<T>()->initialize_associations_in_object(&object, &ctx);
  }
}

#endif // PERSISTENCE_RECORD_TYPE_INCLUDED

#pragma once
#ifndef P_HPP_INCLUDED
#define P_HPP_INCLUDED

#include <string>
#include <algorithm>
#include <cstring>

namespace p {
  struct IType {
    virtual std::string name() const = 0;
  };

  struct IRecordType : IType {
    virtual std::string relation() const = 0;
  };

  struct RecordTypeBase : IRecordType {
    // IType
    std::string name() const final { return name_; }

    // IRecordType
    std::string relation() const final { return relation_; }

    // Internal
    std::string name_;
    std::string relation_;
  };

  template <typename RT>
  struct RecordTypeImpl : RecordTypeBase {
    // TODO: Constructors, destructors, etc.
  };

  template <typename AssociatedType>
  struct BelongsTo {};

  template <typename AssociatedType>
  struct HasMany {};

  template <typename AssociatedType>
  struct HasOne {};

  template <typename RT>
  struct RecordTypeBuilder {
    RecordTypeImpl<RT>* type_;
    RecordTypeImpl<RT>* result_() const { return type_; }

    void name(std::string name) { type_->name_ = std::move(name); }
    void relation(std::string table_name) { type_->relation_ = std::move(table_name); }

    template <typename MemberType>
    void property(MemberType RT::*member, std::string column_name) {}
    template <typename AssociatedType>
    void belongs_to(BelongsTo<AssociatedType> RT::*member, std::string key_column) {}
    template <typename AssociatedType>
    void has_many(HasMany<AssociatedType> RT::*member, std::string foreign_key) {}
  };

  template <typename T> struct BuildType;

  template <typename T>
  auto get_type() -> const decltype(BuildType<T>::build()) {
    static const auto t = BuildType<T>::build();
    return t;
  }
}

#define PERSISTENCE(TYPE) \
  struct RecordTypeBuilder_ ## TYPE : p::RecordTypeBuilder<TYPE> { \
    void build_(); \
    void build_with_defaults_() { \
      name(#TYPE); \
      std::string default_relation_name = #TYPE; \
      std::transform(default_relation_name.begin(), default_relation_name.end(), default_relation_name.begin(), ::tolower); \
      default_relation_name += "s"; \
      relation(default_relation_name); \
      build_(); \
    } \
  }; \
  template <> struct p::BuildType<TYPE> { \
    static p::IRecordType* build() { \
      RecordTypeBuilder_ ## TYPE builder; \
      builder.type_ = new p::RecordTypeImpl<TYPE>; \
      builder.build_with_defaults_(); \
      return builder.result_(); \
    }\
  }; \
  inline void RecordTypeBuilder_ ## TYPE ::build_()

#endif

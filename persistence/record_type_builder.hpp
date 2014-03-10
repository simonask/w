#pragma once
#ifndef PERSISTENCE_RECORD_TYPE_BUILDER_HPP_INCLUDED
#define PERSISTENCE_RECORD_TYPE_BUILDER_HPP_INCLUDED

namespace persistence {
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
}

#endif // PERSISTENCE_RECORD_TYPE_BUILDER_HPP_INCLUDED


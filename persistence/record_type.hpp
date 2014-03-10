#pragma once
#ifndef PERSISTENCE_RECORD_TYPE_INCLUDED
#define PERSISTENCE_RECORD_TYPE_INCLUDED

#include <persistence/type.hpp>

namespace persistence {
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
}

#endif // PERSISTENCE_RECORD_TYPE_INCLUDED

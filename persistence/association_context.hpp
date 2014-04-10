#pragma once
#ifndef PERSISTENCE_ASSOCIATION_CONTEXT_HPP_INCLUDED
#define PERSISTENCE_ASSOCIATION_CONTEXT_HPP_INCLUDED

#include <functional>

namespace persistence {
  // An AssociationContext owns all records from all datastores that have been queried inside it.
  // A record must *not* outlive its context, and associations cannot be populated with records
  // from other contexts.
  struct AssociationContext {
    // template <typename T> T release(RecordPtr<T> record); //

    static AssociationContext* current();
  };

  void push_context(std::function<void()> callback);
}

#endif // PERSISTENCE_ASSOCIATION_CONTEXT_HPP_INCLUDED

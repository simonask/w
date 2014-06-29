#pragma once
#ifndef PERSISTENCE_CONTEXT_HPP_INCLUDED
#define PERSISTENCE_CONTEXT_HPP_INCLUDED

#include <memory>
#include <deque>
#include <map>

#include <persistence/record_ptr.hpp>
#include <persistence/record_type.hpp>

namespace persistence {
  struct ContextLifetimeSentinel {};

  struct Context {
    Context() : sentinel_(std::make_shared<ContextLifetimeSentinel>()) {}
    ~Context();

    struct IPool {
      virtual ~IPool() {}
    };

    template <typename T>
    struct Pool : IPool {
      std::deque<std::unique_ptr<T>> objects_;
    };

    template <typename T>
    RecordPtr<T> create() {
      auto it = pools_.find(get_type<T>());
      Pool<T>* pool = nullptr;
      if (it == pools_.end()) {
        pool = new Pool<T>();
        pools_[get_type<T>()] = std::unique_ptr<IPool>(pool);
      } else {
        pool = dynamic_cast<Pool<T>*>(it->second.get());
      }
      auto ptr = std::unique_ptr<T>(new T);
      get_type<T>()->initialize_associations_in_object(ptr.get(), this);
      RecordPtr<T> rptr { ptr.get(), sentinel_ };
      pool->objects_.push_back(std::move(ptr));
      return std::move(rptr);
    }

    void clear() {
      pools_.clear();
    }

  private:
    using PoolMap = std::map<const IRecordType*, std::unique_ptr<IPool>>;
    PoolMap pools_;
    std::shared_ptr<ContextLifetimeSentinel> sentinel_;
  };

  struct LifetimeError : wayward::Error {
    LifetimeError(const std::string& msg) : wayward::Error(msg) {}
  };

  inline Context::~Context() {
    clear(); // Clear first to delete any RecordPtrs in associations.
    auto count = sentinel_.use_count() - 1;
    if (count > 0) {
      throw LifetimeError(wayward::format("{0} RecordPtr{1} outlived the context holding the record. Check that you aren't saving RecordPtr in global state.", count, (count == 1) ? "" : "s"));
    }
  }
}

#endif // PERSISTENCE_CONTEXT_HPP_INCLUDED

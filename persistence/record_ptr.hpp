#pragma once
#ifndef PERSISTENCE_RECORD_PTR_HPP_INCLUDED
#define PERSISTENCE_RECORD_PTR_HPP_INCLUDED

#include <memory>

namespace persistence {
  struct ContextLifetimeSentinel;

  template <typename T>
  struct RecordPtr {
    RecordPtr() {}
    RecordPtr(std::nullptr_t*) {}
    RecordPtr(const RecordPtr<T>& other) = default;
    RecordPtr(RecordPtr<T>&& other) = default;
    RecordPtr& operator=(const RecordPtr&) = default;
    RecordPtr& operator=(RecordPtr&&) = default;

    bool operator==(const RecordPtr<T>& other) const { return record_ == other.record_; }
    bool operator!=(const RecordPtr<T>& other) const { return record_ != other.record_; }
    explicit operator bool() const { return record_; }

    T* operator->() const { return record_; }
    T& operator*() const { return *record_; }
    T* get() const { return record_; }
  private:
    friend struct Context;
    RecordPtr(T* record, std::shared_ptr<ContextLifetimeSentinel> sentinel) : record_(record), sentinel_(std::move(sentinel)) {}

    T* record_ = nullptr;
    std::shared_ptr<ContextLifetimeSentinel> sentinel_;
  };
}

#endif // PERSISTENCE_RECORD_PTR_HPP_INCLUDED

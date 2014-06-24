#include "wayward/support/any.hpp"

namespace wayward {
  Any::Any(const Any& other) : type_info_(other.type_info_) {
    ensure_allocation();
    type_info_->copy_construct(memory(), other.memory());
  }

  Any::Any(Any&& other) : type_info_(other.type_info_) {
    ensure_allocation();
    type_info_->move_construct(memory(), other.memory());
  }

  Any& Any::operator=(const Any& other) {
    if (type_info_ == other.type_info_) {
      type_info_->copy_assign(memory(), other.memory());
    } else {
      destruct();
      type_info_ = other.type_info_;
      ensure_allocation();
      type_info_->copy_construct(memory(), other.memory());
    }
    return *this;
  }

  Any& Any::operator=(Any&& other) {
    if (type_info_ == other.type_info_) {
      type_info_->move_assign(memory(), other.memory());
    } else {
      destruct();
      type_info_ = other.type_info_;
      ensure_allocation();
      type_info_->move_construct(memory(), other.memory());
    }
    return *this;
  }

  bool Any::is_small_object() const {
    return type_info_->size <= SmallObjectStorageSize && type_info_->alignment <= SmallObjectStorageAlignment;
  }

  void Any::ensure_allocation() {
    if (heap_storage_ == nullptr && !is_small_object()) {
      // TODO: Handle alignment!
      heap_storage_ = reinterpret_cast<void*>(new char[type_info_->size]);
    }
  }

  void Any::destruct() {
    auto mem = memory();
    type_info_->destruct(mem);
    if (mem && !is_small_object()) {
      delete[] reinterpret_cast<char*>(mem);
    }
  }

  void* Any::memory() {
    if (is_small_object()) {
      return &inline_storage_;
    } else {
      return heap_storage_;
    }
  }

  const void* Any::memory() const {
    if (is_small_object()) {
      return &inline_storage_;
    } else {
      return heap_storage_;
    }
  }
}

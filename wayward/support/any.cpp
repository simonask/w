#include "wayward/support/any.hpp"

namespace wayward {
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

  void Any::ensure_allocation() {
    if (!is_inline_storage()) {
      // TODO: Allocate with alignment!!
      heap_storage_ = reinterpret_cast<void*>(new char[type_info_->size]);
    }
  }

  void Any::destruct() {
    if (type_info_) {
      type_info_->destruct(memory());
      if (!is_inline_storage()) {
        delete[] reinterpret_cast<char*>(heap_storage_);
        heap_storage_ = nullptr;
      }
    }
    type_info_ = nullptr;
  }

  void Any::swap(Any& other) {
    Any tmp = std::move(other);
    other = std::move(*this);
    *this = std::move(tmp);
  }
}

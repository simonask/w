#pragma once
#ifndef WAYWARD_SUPPORT_THREAD_LOCAL_HPP_INCLUDED
#define WAYWARD_SUPPORT_THREAD_LOCAL_HPP_INCLUDED

#include <pthread.h>

namespace wayward {
  // WARNING: This presumes that std::thread is implemented in terms of pthread.
  // TODO: Replace with thread_local once libc++ catches up to C++11.
  template <typename T>
  struct ThreadLocal {
    pthread_key_t key;
    ThreadLocal() {
      pthread_key_create(&key, ThreadLocal<T>::destroy);
    }
    ~ThreadLocal() {
      pthread_key_delete(key);
    }

    T* get() {
      T* ptr = reinterpret_cast<T*>(pthread_getspecific(key));
      if (ptr == nullptr) {
        ptr = new T{}; // Important: initializing with empty brackets to set pointer threadlocals to NULL as the default.
        pthread_setspecific(key, reinterpret_cast<void*>(ptr));
      }
      return ptr;
    }

    T& operator*() {
      return *get();
    }

    T* operator->() {
      return get();
    }

    static void destroy(void* ptr) {
      delete reinterpret_cast<T*>(ptr);
    }
  };
}

#endif // WAYWARD_SUPPORT_THREAD_LOCAL_HPP_INCLUDED

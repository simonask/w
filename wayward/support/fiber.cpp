#include <wayward/support/fiber.hpp>

#include <exception>
#include <assert.h>
#include <setjmp.h>
#include <sys/mman.h>

namespace wayward {
  static const size_t FIBER_STACK_SIZE = 1024 * 1024; // Includes canary page.
  static const size_t CANARY_PAGE_SIZE = 4096;        // Minimum page size.
  static const bool   STACK_GROWS_DOWN = true;        // XXX: Should probably be arch-dependent, but we're bound to x86-64 for now anyway.

  enum class FiberSignal {
    Resume,
    Terminate,
  };

  using namespace fiber;

  struct Fiber {
    explicit Fiber(Function function);
    Fiber(Function function, ErrorHandler error_handler);
    Fiber(Fiber&&) = default;
    Fiber();

    bool is_main() const { return !function; }

    FiberPtr invoker;

    jmp_buf portal;
    void* stack = nullptr;
    Function function;
    ErrorHandler error_handler;
    bool started = false;
    bool being_deleted = false;
    FiberSignal sig = FiberSignal::Resume;
  };

  namespace {
    void fiber_delete(Fiber* fiber) {
      /*
        So this is a bit weird and convoluted.
        When a fiber gets deleted, we need to terminate it. However, the Fiber API
        uses unique pointer semantics, and the this-pointer can never be converted to
        a unique_ptr.
        So we use the Deleter of the FiberPtr to flag the fiber for deletion, and then
        we create another FiberPtr from it that we use to call terminate(). When that FiberPtr
        exits scope, the Deleter will be called again, but this time the being_deleted flag
        is already set, so it knows to do nothing.
      */
      if (fiber->started && !fiber->being_deleted && !fiber->is_main()) {
        fiber->being_deleted = true;
        FiberPtr f { fiber };
        fiber::terminate(f);
      } else {
        delete fiber;
      }
    }
  }

  Fiber::Fiber(Function function) : function{std::move(function)} {}

  Fiber::Fiber(Function function, ErrorHandler error_handler) : function(std::move(function)), error_handler(std::move(error_handler)) {}

  Fiber::Fiber() {
    // This is the main fiber! We're being initialized in Fiber::current().
  }

  // WARNING: This presumes that std::thread is implemented in terms of pthread.
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
        ptr = new T;
        pthread_setspecific(key, reinterpret_cast<void*>(ptr));
      }
      return ptr;
    }

    T& operator=(T value) {
      return *get() = std::move(value);
    }

    operator T&() {
      return *get();
    }

    auto operator->() -> decltype(std::declval<T*>()->operator->()) {
      return get()->operator->();
    }

    template <typename U>
    auto operator==(U&& other) -> decltype(std::declval<T>() == std::forward<U>(other)) {
      return *get() == std::forward<U>(other);
    }

    template <typename U>
    auto operator!=(U&& other) -> decltype(std::declval<T>() != std::forward<U>(other)) {
      return *get() != std::forward<U>(other);
    }

    static void destroy(void* ptr) {
      delete reinterpret_cast<T*>(ptr);
    }
  };

  namespace {
    static ThreadLocal<FiberPtr> g_current_fiber; // TODO: Thread-Local

    void free_stack(void* p) {
      ::munmap(p, FIBER_STACK_SIZE);
    }

    void* allocate_stack() {
      void* stack = ::mmap(nullptr, FIBER_STACK_SIZE, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);

      // Mark the 'farthest' (first or last, depending on stack direction) with forbidden access,
      // to ensure that stack overflow results in a crash, and not heap corruption.
      void* canary;
      if (STACK_GROWS_DOWN) {
        canary = stack;
      } else {
        canary = (void*)((intptr_t)stack - CANARY_PAGE_SIZE);
      }
      assert((((intptr_t)canary) % CANARY_PAGE_SIZE) == 0); // Canary page is not on a page bound.
      ::mprotect(canary, CANARY_PAGE_SIZE, PROT_NONE);

      return stack;
    }

    void prepare_jump_into(FiberPtr fiber, FiberSignal sig) {
      fiber->sig = sig;
      fiber->invoker = g_current_fiber;
      g_current_fiber = fiber;
    }

    void handle_return() {
      // Clean-up a terminated fiber if necessary.
      if (!g_current_fiber->invoker->started) {
        free_stack(g_current_fiber->invoker->stack);
        g_current_fiber->invoker->stack = nullptr;
        g_current_fiber->invoker = nullptr;
      }

      if (g_current_fiber->sig == FiberSignal::Terminate) {
        throw FiberTermination{};
      }
    }

    void fiber_trampoline(Fiber*);

    void resume_fiber_with_signal(FiberPtr f, FiberSignal sig) {
      auto current = fiber::current();
      if (setjmp(current->portal) == 0) {
        // There...
        prepare_jump_into(f, sig);
        if (f->started) {
          longjmp(f->portal, 1);
        } else {
          assert(f->stack == nullptr);
          f->stack = allocate_stack();
          f->started = true;

          // Set up stack and jump into fiber:
          void* sp;
          if (STACK_GROWS_DOWN) {
            sp = (void*)((intptr_t)f->stack + FIBER_STACK_SIZE);
          } else {
            sp = f->stack;
          }

          #if defined(__x86_64__)
          __asm__ __volatile__
          (
           "movq %%rsp, %%rbx\n"
           "movq %0, %%rsp\n"
           "movq %1, %%rdi\n"
           "callq *%2\n"
           "movq %%rbx, %%rsp\n"
           : // output registers
           : "r"(sp), "r"(f.get()), "r"(fiber_trampoline) // input registers
           : "rsp", "rbx", "rdi" // clobbered
           );
          #else
          #error Fibers are not supported yet on this platform. :(
          #endif
        }
      } else {
        // and back.
        handle_return();
      }
    }

    void fiber_trampoline(Fiber* f) {
      try {
        f->function();
      }
      catch (const FiberTermination& termination) {
      }
      catch (...) {
        auto eptr = std::current_exception();
        if (f->error_handler) {
          try {
            f->error_handler(std::current_exception());
          }
          catch (...) {
            fprintf(stderr, "Uncaught exception in fiber error handler!");
            std::abort();
          }
        } else {
          fprintf(stderr, "Uncaught exception in fiber!");
          std::abort();
        }
      }
      f->started = false;

      if (f->invoker == nullptr) {
        throw FiberError{"Orphan fiber returned. This means that the fiber was last resumed from a fiber that has now terminated, and can't naturally return to it."};
      }
      resume_fiber_with_signal(std::move(f->invoker), FiberSignal::Resume);
    }
  }

  namespace fiber {
    FiberPtr current() {
      if (g_current_fiber == nullptr) {
        // This is the first time fiber::current() is invoked in this thread,
        // and we haven't yet created a fiber representation of the current main.

        g_current_fiber = FiberPtr{new Fiber};
        g_current_fiber->started = true;
      }
      return g_current_fiber;
    }

    FiberPtr create(Function function) {
      return FiberPtr{new Fiber{std::move(function)}, fiber_delete};
    }

    FiberPtr create(Function function, ErrorHandler error_handler) {
      return FiberPtr{new Fiber{std::move(function), std::move(error_handler)}, fiber_delete};
    }

    void resume(FiberPtr fiber) {
      resume_fiber_with_signal(std::move(fiber), FiberSignal::Resume);
    }

    void start(Function function) {
      auto f = create(std::move(function));
      resume(std::move(f));
    }

    void terminate(FiberPtr fiber) {
      if (fiber->started) {
        resume_fiber_with_signal(std::move(fiber), FiberSignal::Terminate);
      }
    }

    void yield() {
      if (g_current_fiber == nullptr || g_current_fiber->invoker == nullptr) {
        throw FiberError{"Called yield from orphaned fiber."};
      }
      resume_fiber_with_signal(std::move(g_current_fiber->invoker), FiberSignal::Resume);
    }
  }
}

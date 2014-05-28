#include <wayward/support/fiber.hpp>
#include <wayward/support/thread_local.hpp>

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
    ~Fiber();

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

  Fiber::Fiber(Function function) : function{std::move(function)} {
    //printf("Fiber::Fiber()\n");
  }

  Fiber::Fiber(Function function, ErrorHandler error_handler) : function(std::move(function)), error_handler(std::move(error_handler)) {
    //printf("Fiber::Fiber()\n");
  }

  Fiber::Fiber() {
    //printf("Fiber::Fiber()\n");
    // This is the main fiber! We're being initialized in Fiber::current().
  }

  Fiber::~Fiber() {
    //printf("Fiber::~Fiber()\n");
  }

  namespace {
    struct FiberStackAllocator {
      void* allocate_stack() {
        if (free_list == nullptr) {
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
        } else {
          void* next_in_line = *free_list_location(free_list);
          void* ptr = free_list;
          free_list = next_in_line;
          return ptr;
        }
      }

      void free_stack(void* stack) {
        // TODO: Set an upper limit to how many fiber stacks we keep around, perhaps?
        *free_list_location(stack) = free_list;
        free_list = stack;
      }

      ~FiberStackAllocator() {
        void* ptr = free_list;
        while (ptr) {
          void* next_in_line = *((void**)free_list);
          ::munmap(ptr, FIBER_STACK_SIZE);
          ptr = next_in_line;
        }
      }

      void** free_list_location(void* stack) {
        if (STACK_GROWS_DOWN) {
          // We can't just place it at the beginning, because there's a canary there.
          return reinterpret_cast<void**>(reinterpret_cast<char*>(stack) + CANARY_PAGE_SIZE);
        } else {
          return (void**)stack;
        }
      }

      void* free_list = nullptr;
    };

    static ThreadLocal<FiberPtr> g_current_fiber;
    static ThreadLocal<FiberStackAllocator> g_fiber_stack_allocator;

    void prepare_jump_into(FiberPtr fiber, FiberSignal sig) {
      fiber->sig = sig;
      fiber->invoker = std::move(*g_current_fiber);
      *g_current_fiber = std::move(fiber);
    }

    void handle_return() {
      // Clean-up a terminated fiber if necessary.
      FiberPtr& current = *g_current_fiber;
      assert(current);
      if (!current->invoker->started) {
        g_fiber_stack_allocator.get()->free_stack(current->invoker->stack);
        current->invoker->stack = nullptr;
        current->invoker = nullptr;
      }

      if (current->sig == FiberSignal::Terminate) {
        throw FiberTermination{};
      }
    }

    void fiber_trampoline(Fiber*);

    void resume_fiber_with_signal(FiberPtr f, FiberSignal sig) {
      auto current = fiber::current();
      if (setjmp(current->portal) == 0) {
        current = nullptr; // Reset this so we don't end up holding a reference to ourselves.
        // There...
        prepare_jump_into(f, sig);
        if (f->started) {
          longjmp(f->portal, 1);
        } else {
          assert(f->stack == nullptr);
          f->stack = g_fiber_stack_allocator->allocate_stack();
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
      if (*g_current_fiber == nullptr) {
        // This is the first time fiber::current() is invoked in this thread,
        // and we haven't yet created a fiber representation of the current main.

        *g_current_fiber = FiberPtr{new Fiber};
        (*g_current_fiber)->started = true;
      }
      return *g_current_fiber;
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
      if (*g_current_fiber == nullptr || (*g_current_fiber)->invoker == nullptr) {
        throw FiberError{"Called yield from orphaned fiber."};
      }
      resume_fiber_with_signal(std::move((*g_current_fiber)->invoker), FiberSignal::Resume);
    }
  }
}

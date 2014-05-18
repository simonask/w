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

  struct FiberTermination {};

  struct Fiber::Private {
    jmp_buf portal;
    void* stack = nullptr;
    Function function;
    ErrorHandler error_handler;
    bool started = false;
    FiberSignal sig = FiberSignal::Resume;
    Fiber* resumed_from = nullptr;
    Fiber* on_exit = nullptr;

    Private() {}
    explicit Private(Function function) : function(std::move(function)) {}
  };

  Fiber::Fiber(Function function) : p_(new Private{std::move(function)}) {}

  Fiber::Fiber(Function function, ErrorHandler error_handler) : Fiber(std::move(function)) {
    set_error_handler(std::move(error_handler));
  }

  Fiber::Fiber() {
    // This is the main fiber! We're being initialized in Fiber::current().
  }

  Fiber::~Fiber() {
    // Call terminate unless we're a "root" fiber (main or thread).
    if (p_->function) {
      terminate();
    }
    assert(p_->stack == nullptr);
  }

  void Fiber::set_error_handler(ErrorHandler error_handler) {
    p_->error_handler = std::move(error_handler);
  }

  namespace {
    static __thread Fiber* g_current_fiber = nullptr;
    // Poor man's TLS:
    static __thread char g_fiber_storage[sizeof(Fiber)];
    static __thread char g_fiber_storage_private[sizeof(Fiber::Private)];
  }

  Fiber& Fiber::current() {
    if (g_current_fiber == nullptr) {
      // We are being called from a new thread for the first time.
      g_current_fiber = new(g_fiber_storage) Fiber;
      // This is a bit hacky -- the destructor for Fiber will never get called, because we're not using
      // proper C++11 TLS, but since all fields are empty in the "main" fiber, this shouldn't leak any
      // resources.
      g_current_fiber->p_ = std::unique_ptr<Fiber::Private>(new(g_fiber_storage_private) Fiber::Private);
      auto p = g_current_fiber->p_.get();
      p->started = true;
    }

    return *g_current_fiber;
  }

  namespace {
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

    void handle_return_from_fiber(Fiber* f) {
      Fiber& self = Fiber::current();
      Fiber::Private* p = f->p_.get();
      if (!p->started) {
        // Fiber returned, so clean it up!
        free_stack(p->stack);
        p->stack = nullptr;
      }

      // Check if we're been told to terminate.

      Fiber::Private* p_self = self.p_.get();
      if (p_self->sig == FiberSignal::Terminate) {
        throw FiberTermination{};
      }
    }

    void resume_fiber_with_signal(Fiber* f, FiberSignal sig) {
      Fiber& self = Fiber::current();
      if (&self == f)
        return;

      Fiber::Private* p_self = self.p_.get();

      if (setjmp(p_self->portal) == 0) {
        Fiber::Private* p = f->p_.get();
        p->sig = sig;
        p->resumed_from = &self;
        g_current_fiber = f;
        longjmp(p->portal, 1);
      } else {
        handle_return_from_fiber(self.p_->resumed_from);
      }
    }

    void fiber_trampoline(Fiber* f) {
      Fiber::Private* p = f->p_.get();
      try {
        p->function();
      }
      catch (const FiberTermination& termination) {
      }
      catch (...) {
        auto eptr = std::current_exception();
        if (p->error_handler) {
          try {
            p->error_handler(std::current_exception());
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
      p->started = false;

      if (p->on_exit == nullptr) {
        fprintf(stderr, "Unowned fiber returned!");
        std::abort();
      }
      resume_fiber_with_signal(p->on_exit, FiberSignal::Resume);
    }

    void start_fiber(Fiber* f) {
      Fiber& self = Fiber::current();
      Fiber::Private* p_self = self.p_.get();

      if (setjmp(p_self->portal) == 0) {
        Fiber::Private* p = f->p_.get();
        p->on_exit = &self; // Become the 'owner' of the new fiber.

        assert(p->stack == nullptr);
        p->stack = allocate_stack();
        p->started = true;
        p->sig = FiberSignal::Resume;
        p->resumed_from = &self;
        g_current_fiber = f;

        // Set up stack and jump into fiber:
        void* sp;
        if (STACK_GROWS_DOWN) {
          sp = (void*)((intptr_t)p->stack + FIBER_STACK_SIZE);
        } else {
          sp = p->stack;
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
         : "r"(sp), "r"(f), "r"(fiber_trampoline) // input registers
         : "rsp", "rbx", "rdi" // clobbered
         );
        #else
        #error Fibers are not supported yet on this platform. :(
        #endif
      } else {
        handle_return_from_fiber(self.p_->resumed_from);
      }
    }
  }

  void Fiber::terminate() {
    if (p_->started) {
      p_->on_exit = &Fiber::current(); // Make sure that we get resumed when the fiber is done terminating.
      resume_fiber_with_signal(this, FiberSignal::Terminate);
    }
  }

  void Fiber::resume() {
    if (p_->started) {
      resume_fiber_with_signal(this, FiberSignal::Resume);
    } else {
      start_fiber(this);
    }
  }
}

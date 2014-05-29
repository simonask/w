#include <wayward/support/error.hpp>

#if defined(__APPLE__)
#include <execinfo.h>
#include <libunwind.h>
#include <dlfcn.h>
#include <cxxabi.h>
#elif defined(__linux__)
#define UNW_LOCAL_ONLY 1
#include <libunwind.h>
#include <cxxabi.h>
#endif

namespace wayward {
  #if defined(__APPLE__) || defined(__linux__)

  std::string demangle_symbol(const std::string& mangled) {
    size_t len;
    int status;
    char* buffer = __cxxabiv1::__cxa_demangle(mangled.c_str(), nullptr, &len, &status);
    std::string result;
    if (status == 0) {
      result = buffer;
    } else {
      result = mangled;
    }
    ::free(buffer);
    return std::move(result);
  }

  template <typename OutputIterator>
  size_t get_backtrace(OutputIterator line_inserter, size_t num_steps, size_t offset) {
    offset += 1;
    unw_cursor_t ucur;
    unw_context_t uctx;
    unw_getcontext(&uctx);
    unw_init_local(&ucur, &uctx);
    size_t i = 0;
    while (unw_step(&ucur) > 0) {
      if (i >= offset) {
        size_t j = i - offset;
        // unw_word_t val;
        // unw_get_reg(&ucur, UNW_REG_IP, &val);
        // out_instruction_pointers[j] = (void*)val;
        std::array<char, 512> name_buffer;
        unw_word_t entry_offset;
        unw_get_proc_name(&ucur, name_buffer.data(), name_buffer.size(), &entry_offset);
        *line_inserter = wayward::format("{0} + {1}", demangle_symbol(std::string(name_buffer.data())), entry_offset);
        ++line_inserter;
      }
      ++i;
      if (i >= num_steps + offset)
        break;
    }
    return i - offset;
  }

  std::vector<std::string> current_backtrace() {
    std::vector<std::string> lines;
    get_backtrace(std::back_inserter(lines), 10, 2);
    return std::move(lines);
  }

  #else
  #warning No backtrace support for this platform.
  std::vector<std::string> current_backtrace() {
    return {};
  }
  #endif
}

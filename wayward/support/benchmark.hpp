#pragma once
#ifndef WAYWARD_SUPPORT_BENCHMARK_HPP_INCLUDED
#define WAYWARD_SUPPORT_BENCHMARK_HPP_INCLUDED

#include <wayward/support/datetime.hpp>
#include <map>
#include <string>
#include <functional>

namespace wayward {
  struct PerformanceClock : IClock {
    DateTime now() const final;
    Timezone timezone() const final;

    static PerformanceClock& get();
  private:
    PerformanceClock() {}
    ~PerformanceClock() {}
  };

  struct Benchmark {
    Benchmark();
    ~Benchmark();

    void start();
    DateTimeInterval finish();

    static DateTimeInterval measure(std::function<void()>);

    DateTimeInterval total() const;
    std::map<std::string, DateTimeInterval> scopes() const;

    void enter_scope(std::string name);
    void exit_scope(std::string name);
  private:
    struct Measurement {
      DateTimeInterval accum;
      DateTime begin;
      size_t activations = 0;
    };

    Measurement total_;
    std::map<std::string, Measurement> scopes_;
  };

  struct BenchmarkScope {
    Benchmark& bm_;
    std::string name_;
    BenchmarkScope(Benchmark& bm, std::string name) : bm_(bm), name_(std::move(name)) {
      bm_.enter_scope(name_);
    }
    ~BenchmarkScope() {
      bm_.exit_scope(name_);
    }
  };
}

#endif // WAYWARD_SUPPORT_BENCHMARK_HPP_INCLUDED

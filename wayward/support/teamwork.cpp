#include "wayward/support/teamwork.hpp"

#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>


namespace wayward {
  struct Teamwork::Private {
    std::vector<std::thread> workers_;
    std::condition_variable work_cond_;
    std::mutex mutex_;
    std::queue<Teamwork::Function> queue_;
    volatile bool exiting = false;

    void work_thread(int n) {
      std::unique_lock<std::mutex> L(mutex_);
      while (!exiting) {
        while (queue_.empty() && !exiting) {
          work_cond_.wait(L);
        }
        if (!exiting && !queue_.empty()) {
          auto function = queue_.front();
          queue_.pop();
          L.unlock();
          function();
          L.lock();
        }
      }
    }
  };

  Teamwork::Teamwork(size_t num_workers) : p_(new Private) {
    for (size_t i = 0; i < num_workers; ++i) {
      Private* p = p_.get();
      p_->workers_.emplace_back([=](){ p->work_thread((int)i); });
    }
  }

  Teamwork::~Teamwork() {
    p_->exiting = true;
    // Wake up *all* workers by posting N signals.
    p_->work_cond_.notify_all();
    // Wait for all workers to exit.
    for (auto& pw: p_->workers_) {
      pw.join();
    }
  }

  void Teamwork::work(Teamwork::Function f) {
    std::unique_lock<std::mutex> L(p_->mutex_);
    p_->queue_.push(std::move(f));
    p_->work_cond_.notify_one();
  }
}

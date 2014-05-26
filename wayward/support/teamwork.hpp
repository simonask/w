#pragma once
#ifndef WAYWARD_SUPPORT_TEAMWORK_HPP_INCLUDED
#define WAYWARD_SUPPORT_TEAMWORK_HPP_INCLUDED

#include <functional>

namespace wayward {
  struct Teamwork {
    explicit Teamwork(size_t num_workers = 4);
    ~Teamwork();

    using Function = std::function<void()>;

    void work(Function func);

    size_t number_of_workers() const;

    struct Private;
    std::unique_ptr<Private> p_;
  };
}

#endif // WAYWARD_SUPPORT_TEAMWORK_HPP_INCLUDED

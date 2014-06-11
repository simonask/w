#pragma once
#ifndef PERSISTENCE_CREATE_HPP_INCLUDED
#define PERSISTENCE_CREATE_HPP_INCLUDED

#include <persistence/record_ptr.hpp>
#include <persistence/context.hpp>
#include <persistence/assign_attributes.hpp>

namespace persistence {
  template <typename T>
  RecordPtr<T> create(Context& ctx, const wayward::data_franca::Spectator& data = wayward::data_franca::Spectator{}) {
    auto record = ctx.create<T>();
    assign_attributes(record, data);
    return std::move(record);
  }
}

#endif // PERSISTENCE_CREATE_HPP_INCLUDED

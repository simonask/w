#pragma once
#ifndef PERSISTENCE_CREATE_HPP_INCLUDED
#define PERSISTENCE_CREATE_HPP_INCLUDED

#include <persistence/record_ptr.hpp>
#include <persistence/context.hpp>
#include <wayward/support/data_franca/spelunker.hpp>

namespace persistence {
  template <typename T>
  RecordPtr<T> create(Context&, const wayward::data_franca::Spelunker& data = wayward::data_franca::Spelunker{});
}

#endif // PERSISTENCE_CREATE_HPP_INCLUDED

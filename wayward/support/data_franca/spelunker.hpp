#pragma once
#ifndef WAYWARD_SUPPORT_DATA_FRANCA_SPELUNKER_HPP_INCLUDED
#define WAYWARD_SUPPORT_DATA_FRANCA_SPELUNKER_HPP_INCLUDED

#include <wayward/support/data_franca/reader.hpp>
#include <wayward/support/data_franca/get_reader.hpp>

namespace wayward {
  namespace data_franca {
    /*
      A Spelunker can spelunk through data structures of any type that defines a
      suitable adapter.
    */
    struct Spelunker final : ReaderInterface<Spelunker> {
      Spelunker() {}
      Spelunker(const Spelunker&) = default;
      Spelunker(Spelunker&&) = default;

      Spelunker& operator=(const Spelunker&) = default;
      Spelunker& operator=(Spelunker&&) = default;

      template <typename T>
      Spelunker(T&& object);

      DataType type() const { return q_ ? q_->type() : DataType::NothingType; }

      const IReader& reader_iface() const { return q_ ? *q_ : g_null_reader; }
    private:
      ReaderPtr q_;
      friend struct ReaderInterface<Spelunker>;
      friend struct iterator;
      Spelunker(ReaderPtr ptr) : q_{std::move(ptr)} {}

      static const NullReader g_null_reader;
    };

    template <typename T>
    Spelunker::Spelunker(T&& object) : q_(make_reader(std::forward<T>(object))) {}
  }
}

#endif // WAYWARD_SUPPORT_DATA_FRANCA_SPELUNKER_HPP_INCLUDED

#pragma once
#ifndef WAYWARD_SUPPORT_DATA_FRANCA_SPELUNKER_HPP_INCLUDED
#define WAYWARD_SUPPORT_DATA_FRANCA_SPELUNKER_HPP_INCLUDED

#include <wayward/support/data_franca/reader.hpp>
#include <wayward/support/data_franca/adapter.hpp>

namespace wayward {
  namespace data_franca {
    /*
      A Spelunker can spelunk through data structures of any type that defines a
      suitable adapter.
    */
    struct Spelunker final : ReaderInterface<Spelunker> {
      Spelunker() {}
      Spelunker(ReaderPtr ptr) : q_{std::move(ptr)} {}
      Spelunker(const Spelunker&) = default;
      Spelunker(Spelunker&&) = default;

      Spelunker& operator=(const Spelunker&) = default;
      Spelunker& operator=(Spelunker&&) = default;

      template <typename T>
      Spelunker(T&& object);

      Spelunker operator[](size_t idx) const { return this->reader_subscript(idx); }
      Spelunker operator[](const String& key) const { return this->reader_subscript(key); }

      DataType type() const { return q_ ? q_->type() : DataType::Nothing; }

      const IReader& reader_iface() const { return q_ ? *q_ : g_null_reader; }
    private:
      ReaderPtr q_;
      friend struct ReaderInterface<Spelunker>;
      friend struct GetAdapter<Spelunker>;

      static const NullReader g_null_reader;
    };

    template <typename T>
    Spelunker::Spelunker(T&& object) : q_(make_reader(std::forward<T>(object))) {}

    template <>
    struct GetAdapter<Spelunker> {
      static ReaderPtr get(const Spelunker& s) { return s.q_; }
    };
  }
}

#endif // WAYWARD_SUPPORT_DATA_FRANCA_SPELUNKER_HPP_INCLUDED

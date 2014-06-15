#pragma once
#ifndef WAYWARD_SUPPORT_DATA_FRANCA_SPELUNKER_HPP_INCLUDED
#define WAYWARD_SUPPORT_DATA_FRANCA_SPELUNKER_HPP_INCLUDED

#include <wayward/support/data_franca/reader.hpp>
#include <wayward/support/data_franca/adapter.hpp>

namespace wayward {
  namespace data_franca {
    /*
      A Spectator can spelunk through data structures of any type that defines a
      suitable adapter.
    */
    struct Spectator final : ReaderInterface<Spectator> {
      Spectator() {}
      Spectator(ReaderPtr ptr) : q_{std::move(ptr)} {}
      Spectator(const Spectator&) = default;
      Spectator(Spectator&&) = default;
      template <typename T>
      Spectator(T&& object);
      template <typename T>
      Spectator(const T& object);
      template <typename T>
      Spectator(T& object);

      Spectator& operator=(const Spectator&) = default;
      Spectator& operator=(Spectator&&) = default;

      explicit operator bool() const { return type() != DataType::Nothing; }

      Spectator operator[](size_t idx) const { return this->reader_subscript(idx); }
      Spectator operator[](const String& key) const { return this->reader_subscript(key); }
      Spectator operator[](const char* key) const { return this->reader_subscript(key); }

      DataType type() const { return q_ ? q_->type() : DataType::Nothing; }

      const IReader& reader_iface() const { return q_ ? *q_ : g_null_reader; }
    private:
      ReaderPtr q_;
      friend struct ReaderInterface<Spectator>;
      friend struct GetAdapter<Spectator>;

      static const NullReader g_null_reader;
    };

    template <typename T>
    Spectator::Spectator(const T& object) : q_(make_reader(object, Options::AllowLoad)) {}

    template <typename T>
    Spectator::Spectator(T& object) : q_(make_reader(object, Options::AllowLoad)) {}

    template <typename T>
    Spectator::Spectator(T&& object) : q_(make_owning_reader(std::move(object), Options::AllowLoad)) {}

    template <>
    struct GetAdapter<Spectator> {
      static ReaderPtr get(const Spectator& s, Bitflags<Options> o) { return s.q_; }
    };

    /*
      The ScalarSpectator has identical semantics to the Spectator, except it can only
      represent scalar values (i.e., not lists and dictionaries). It's an optimization
      to allow some types of deserialization without allocation.
    */
    struct ScalarSpectator final : ReaderInterface<ScalarSpectator, Spectator> {
      ScalarSpectator(const ScalarSpectator&) = default;
      ScalarSpectator(const Spectator& spectator) : reader_(spectator.reader_iface()) { }
      ScalarSpectator(const IReader& reader) : reader_(reader) {}

      explicit operator bool() const { return type() != DataType::Nothing; }

      DataType type() const { return reader_.type(); }

      const IReader& reader_iface() const { return reader_; }
    private:
      friend struct ReaderInterface<ScalarSpectator, Spectator>;
      const IReader& reader_;
    };
  }
}

#endif // WAYWARD_SUPPORT_DATA_FRANCA_SPELUNKER_HPP_INCLUDED

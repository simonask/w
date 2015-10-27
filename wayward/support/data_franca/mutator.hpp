#pragma once
#ifndef WAYWARD_SUPPORT_DATA_FRANCA_MUTATOR_HPP_INCLUDED
#define WAYWARD_SUPPORT_DATA_FRANCA_MUTATOR_HPP_INCLUDED

#include <wayward/support/data_franca/adapter.hpp>
#include <wayward/support/data_franca/spectator.hpp>

namespace wayward {
  namespace data_franca {
    struct Mutator final : ReaderInterface<Mutator, Spectator>, WriterInterface<Mutator> {
      Mutator() {}
      Mutator(AdapterPtr ptr) : q_{std::move(ptr)} {}
      Mutator(const Mutator&) = default;
      Mutator(Mutator&&) = default;
      Mutator& operator=(const Mutator&) = default;
      Mutator& operator=(Mutator&&) = default;

      template <typename T>
      Mutator(T&& object);

      Spectator operator[](size_t idx) const { return this->reader_subscript(idx); }
      Spectator operator[](const String& key) const { return this->reader_subscript(key); }
      Spectator operator[](const char* key) const { return this->reader_subscript(key); }
      Mutator operator[](size_t idx) { return this->writer_subscript(idx); }
      Mutator operator[](const String& key) { return this->writer_subscript(key); }
      Mutator operator[](const char* key) { return this->writer_subscript(key); }

      DataType type() const;

      const IReader& reader_iface() const;
      IWriter& writer_iface();
      IAdapter& adapter_iface();
    private:
      friend struct ReaderInterface<Mutator>;
      friend struct WriterInterface<Mutator>;
      friend struct GetAdapter<Mutator>;

      AdapterPtr q_;

      static const NullReader g_null_reader;
    };

    template <typename T>
    Mutator::Mutator(T&& object) : q_(make_adapter(std::forward<T>(object), Options::None)) {}

    inline DataType Mutator::type() const {
      return reader_iface().type();
    }

    inline const IReader& Mutator::reader_iface() const {
      return q_ ? static_cast<const IReader&>(*q_) : g_null_reader;
    }

    struct MutationError : Error {
      MutationError(const std::string& message) : Error(message) {}
    };

    inline IWriter& Mutator::writer_iface() {
      if (q_ == nullptr) {
        throw MutationError{"Attempted to modify an empty mutator."};
      }
      return *q_;
    }

    inline IAdapter& Mutator::adapter_iface() {
      if (q_ == nullptr) {
        throw MutationError{"Attempted to modify an empty mutator."};
      }
      return *q_;
    }

    template <>
    struct GetAdapter<Mutator> {
      static ReaderPtr get(const Mutator& m, Options options) { return std::static_pointer_cast<const IReader>(m.q_); }
      static AdapterPtr get(Mutator& m, Options options) { return m.q_; }
    };

    struct ScalarMutator final : ReaderInterface<ScalarMutator, Spectator>, WriterInterface<ScalarMutator, Mutator> {
      ScalarMutator(const ScalarMutator&) = default;
      ScalarMutator(Mutator& mutator) : adapter_(mutator.adapter_iface()) {}
      ScalarMutator(IAdapter& adapter) : adapter_(adapter) {}

      explicit operator bool() const { return type() != DataType::Nothing; }
      DataType type() const { return adapter_.type(); }

      const IReader& reader_iface() const { return adapter_; }
      IWriter& writer_iface() { return adapter_; }
      IAdapter& adapter_iface() { return adapter_; }
    private:
      IAdapter& adapter_;
    };
  }
}

#endif // WAYWARD_SUPPORT_DATA_FRANCA_MUTATOR_HPP_INCLUDED

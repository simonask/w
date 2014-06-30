#pragma once
#ifndef PERSISTENCE_PROJECTION_AS_STRUCTURED_DATA_HPP_INCLUDED
#define PERSISTENCE_PROJECTION_AS_STRUCTURED_DATA_HPP_INCLUDED

#include <wayward/support/data_franca/adapter.hpp>
#include <persistence/projection.hpp>

namespace wayward {
  namespace data_franca {
    template <typename Primary, typename Joins>
    struct Adapter<persistence::Projection<Primary, Joins>> : AdapterBase<persistence::Projection<Primary, Joins>> {
      using Proj = persistence::Projection<Primary, Joins>;

      Adapter(Proj& proj, Bitflags<Options> options) : AdapterBase<Proj>(proj, options) {}

      // IReader interface:
      DataType type() const override { return DataType::List; }
      Maybe<Boolean> get_boolean() const final { return Nothing; }
      Maybe<Integer> get_integer() const final { return Nothing; }
      Maybe<Real>    get_real() const final { return Nothing; }
      Maybe<String>  get_string() const final { return Nothing; }
      bool has_key(const String& key) const final { return false; }
      ReaderPtr get(const String& key) const final { return nullptr; }

      size_t length() const final {
        return this->ref_.count();
      }

      ReaderPtr at(size_t idx) const final {
        load();
        return make_reader(values_->at(idx), this->options_);
      }

      ReaderEnumeratorPtr enumerator() const final {
        load();
        return make_reader(*values_, this->options_)->enumerator();
      }

    private:
      mutable Maybe<std::vector<persistence::RecordPtr<Primary>>> values_;

      void load() const {
        if (!values_) {
          values_ = this->ref_.all();
        }
      }
    };

    template <typename Primary, typename Joins>
    struct GetAdapter<persistence::Projection<Primary, Joins>> {
      using Proj = persistence::Projection<Primary, Joins>;
      static AdapterPtr get(Proj& proj, Bitflags<Options> options) {
        return std::static_pointer_cast<IAdapter>(std::make_shared<Adapter<Proj>>(proj, options));
      }
    };
  }
}

#endif // PERSISTENCE_PROJECTION_AS_STRUCTURED_DATA_HPP_INCLUDED

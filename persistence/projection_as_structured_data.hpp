#pragma once
#ifndef PERSISTENCE_PROJECTION_AS_STRUCTURED_DATA_HPP_INCLUDED
#define PERSISTENCE_PROJECTION_AS_STRUCTURED_DATA_HPP_INCLUDED

#include <wayward/support/data_franca/adapter.hpp>
#include <persistence/projection.hpp>

namespace wayward {
  namespace data_franca {
    template <typename Proj> struct ProjectionReader;

    template <typename Primary, typename... Relations>
    struct ProjectionReader<persistence::Projection<Primary, Relations...>> : IReader {
      using Proj = persistence::Projection<Primary, Relations...>;

      ProjectionReader(Proj& proj, Bitflags<Options> options) : proj_(proj), options_(options) {}

      // IReader interface:
      DataType type() const override { return DataType::List; }
      Maybe<Boolean> get_boolean() const final { return Nothing; }
      Maybe<Integer> get_integer() const final { return Nothing; }
      Maybe<Real>    get_real() const final { return Nothing; }
      Maybe<String>  get_string() const final { return Nothing; }
      bool has_key(const String& key) const final { return false; }
      ReaderPtr get(const String& key) const final { return nullptr; }

      size_t length() const final {
        load();
        return proj_.count();
      }

      ReaderPtr at(size_t idx) const final {
        load();
        return make_reader(values_->at(idx), options_);
      }

      ReaderEnumeratorPtr enumerator() const final {
        load();
        return make_reader(*values_, options_)->enumerator();
      }

    private:
      Proj& proj_;
      Bitflags<Options> options_;
      mutable Maybe<std::vector<persistence::RecordPtr<Primary>>> values_;

      void load() const {
        if (!values_) {
          values_ = proj_.all();
        }
      }
    };

    template <typename Primary, typename... Relations>
    struct GetAdapter<persistence::Projection<Primary, Relations...>> {
      using Proj = persistence::Projection<Primary, Relations...>;
      static ReaderPtr get(Proj& proj, Bitflags<Options> options) {
        return std::static_pointer_cast<const IReader>(std::make_shared<ProjectionReader<Proj>>(proj, options));
      }
    };
  }
}

#endif // PERSISTENCE_PROJECTION_AS_STRUCTURED_DATA_HPP_INCLUDED

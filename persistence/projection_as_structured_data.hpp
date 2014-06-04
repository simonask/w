#pragma once
#ifndef PERSISTENCE_PROJECTION_AS_STRUCTURED_DATA_HPP_INCLUDED
#define PERSISTENCE_PROJECTION_AS_STRUCTURED_DATA_HPP_INCLUDED

#include <wayward/support/structured_data.hpp>
#include <persistence/projection.hpp>

namespace persistence {

}

namespace wayward {
  template <typename Primary, typename... Relations>
  struct StructuredDataAdapter<persistence::Projection<Primary, Relations...>> : IStructuredData {
    using Proj = persistence::Projection<Primary, Relations...>;

    explicit StructuredDataAdapter(Proj proj) : proj_(std::move(proj)) {}

    NodeType type() const final {
      return NodeType::List;
    }

    size_t length() const final {
      load();
      return proj_.count();
    }

    std::vector<std::string> keys() const final {
      return std::vector<std::string>{};
    }

    StructuredDataConstPtr get(const std::string& str) const final {
      return nullptr;
    }

    StructuredDataConstPtr get(size_t idx) const final {
      load();
      return make_structured_data_adapter(values->at(idx));
    }

    Maybe<std::string>  get_string() const final { return Nothing; }
    Maybe<int64_t> get_integer() const final { return Nothing; }
    Maybe<double>  get_float() const final { return Nothing; }
    Maybe<bool>    get_boolean() const final { return Nothing; }
  private:
    mutable Proj proj_;
    mutable Maybe<std::vector<persistence::RecordPtr<Primary>>> values;

    void load() const {
      if (!values) {
        values = proj_.all();
      }
    }
  };
}

#endif // PERSISTENCE_PROJECTION_AS_STRUCTURED_DATA_HPP_INCLUDED

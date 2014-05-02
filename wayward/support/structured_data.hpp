#pragma once
#ifndef WAYWARD_SUPPORT_STRUCTURED_DATA_HPP_INCLUDED
#define WAYWARD_SUPPORT_STRUCTURED_DATA_HPP_INCLUDED

#include <memory>
#include <vector>
#include <string>

#include <wayward/support/maybe.hpp>
#include <wayward/support/meta.hpp>

namespace wayward {
  using std::int64_t;

  enum class NodeType {
    Nil,
    Integer,
    Float,
    String,
    List,
    Dictionary,
  };

  struct IStructuredData {
    virtual ~IStructuredData() {}
    virtual NodeType type() const = 0;

    virtual size_t length() const = 0;
    virtual std::vector<std::string> keys() const = 0;

    virtual std::shared_ptr<const IStructuredData>
    operator[](const std::string& str) const = 0;

    virtual std::shared_ptr<const IStructuredData>
    operator[](size_t idx) const = 0;

    virtual Maybe<std::string>  get_string() const = 0;
    virtual Maybe<int64_t> get_integer() const = 0;
    virtual Maybe<double>  get_float() const = 0;
  };

  // Default implementation deliberately left out.
  template <typename T, typename Enable = void> struct StructuredDataAdapter;

  template <typename T>
  std::shared_ptr<const IStructuredData> as_structured_data(T&& object) {
    using Adapter = StructuredDataAdapter<typename meta::RemoveConstRef<T>::Type>;
    auto ptr = std::make_shared<Adapter>(std::forward<T>(object));
    return std::static_pointer_cast<const IStructuredData>(std::move(ptr));
  }
}

#endif // WAYWARD_SUPPORT_STRUCTURED_DATA_HPP_INCLUDED

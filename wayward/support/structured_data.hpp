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
    Boolean,
    Integer,
    Float,
    String,
    List,
    Dictionary,
  };

  struct IStructuredData;
  using StructuredDataConstPtr = std::shared_ptr<const IStructuredData>;

  struct IStructuredData {
    virtual ~IStructuredData() {}
    virtual NodeType type() const = 0;

    virtual size_t length() const = 0;
    virtual std::vector<std::string> keys() const = 0;

    virtual StructuredDataConstPtr get(const std::string& str) const = 0;
    virtual StructuredDataConstPtr get(size_t idx) const = 0;
    virtual Maybe<std::string> get_string() const = 0;
    virtual Maybe<int64_t>     get_integer() const = 0;
    virtual Maybe<double>      get_float() const = 0;
    virtual Maybe<bool>        get_boolean() const = 0;
  };

  // Default implementation deliberately left out.
  template <typename T, typename Enable = void> struct StructuredDataAdapter;

  template <typename T>
  StructuredDataConstPtr make_structured_data_adapter(T&& object) {
    using Adapter = StructuredDataAdapter<typename meta::RemoveConstRef<T>::Type>;
    auto ptr = std::make_shared<Adapter>(std::forward<T>(object));
    return std::static_pointer_cast<const IStructuredData>(std::move(ptr));
  }

  template <typename T> struct GetStructuredDataAdapter;

  template <typename T> struct GetStructuredDataAdapter {
    static auto get(T x) -> decltype(make_structured_data_adapter(x)) {
      return make_structured_data_adapter(std::move(x));
    }
  };
}

#endif // WAYWARD_SUPPORT_STRUCTURED_DATA_HPP_INCLUDED

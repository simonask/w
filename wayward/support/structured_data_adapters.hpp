#pragma once
#ifndef WAYWARD_SUPPORT_STRUCTURED_DATA_ADAPTERS_HPP_INCLUDED
#define WAYWARD_SUPPORT_STRUCTURED_DATA_ADAPTERS_HPP_INCLUDED

#include <wayward/support/structured_data.hpp>
#include <wayward/support/maybe.hpp>
#include <wayward/support/meta.hpp>
#include <map>

namespace wayward {
  template <typename T>
  struct StructuredDataAdapter<Maybe<T>> : IStructuredData {
    StructuredDataAdapter(Maybe<T>&& value) : value_(std::move(value)), access_(value_) {}
    StructuredDataAdapter(const Maybe<T>& value) : access_(value) {}
    StructuredDataAdapter(Maybe<T>& value) : access_(value) {}

    NodeType type() const final {
      return value_ ? as_structured_data(*access_)->type() : NodeType::Nil;
    }

    size_t length() const final {
      return value_ ? as_structured_data(*access_)->length() : 0;
    }

    std::vector<std::string> keys() const final {
      return value_ ? as_structured_data(*access_)->keys() : std::vector<std::string>();
    }

    std::shared_ptr<const IStructuredData>
    operator[](const std::string& key) const final {
      return value_ ? as_structured_data(*access_)->operator[](key) : nullptr;
    }

    std::shared_ptr<const IStructuredData>
    operator[](const size_t idx) const final {
      return value_ ? as_structured_data(*access_)->operator[](idx) : nullptr;
    }

    Maybe<std::string> get_string() const final {
      return value_ ? as_structured_data(*access_)->get_string() : Maybe<std::string>(Nothing);
    }

    Maybe<int64_t> get_integer() const final {
      return value_ ? as_structured_data(*access_)->get_integer() : Maybe<int64_t>(Nothing);
    }

    Maybe<double> get_float() const final {
      return value_ ? as_structured_data(*access_)->get_float() : Maybe<double>(Nothing);
    }

    Maybe<bool> get_boolean() const final {
      return value_ ? as_structured_data(*access_)->get_boolean() : Maybe<bool>(Nothing);
    }
  private:
    Maybe<T> value_;
    const Maybe<T>& access_;
  };

  struct StructuredDataValue : IStructuredData {
    size_t length() const override { return 0; }
    std::vector<std::string> keys() const override { return std::vector<std::string>(); }
    std::shared_ptr<const IStructuredData> operator[](const std::string&) const override { return nullptr; }
    std::shared_ptr<const IStructuredData> operator[](size_t) const override { return nullptr; }
    Maybe<std::string> get_string() const override { return Nothing; }
    Maybe<int64_t> get_integer() const override { return Nothing; }
    Maybe<double> get_float() const override { return Nothing; }
    Maybe<bool> get_boolean() const override { return Nothing; }
  };

  template <>
  struct StructuredDataAdapter<bool> : StructuredDataValue {
    StructuredDataAdapter(bool b) : b_(b) {}
    Maybe<bool> get_boolean() const final { return b_; }
    bool b_;
  };

  struct StructuredDataStringAdapter : StructuredDataValue {
    StructuredDataStringAdapter(std::string str) : string_(std::move(str)) {}
    NodeType type() const final { return NodeType::String; }
    Maybe<std::string> get_string() const final { return string_; }
  private:
    std::string string_;
  };

  template <>
  struct StructuredDataAdapter<std::string> : StructuredDataStringAdapter {
    StructuredDataAdapter(std::string value) : StructuredDataStringAdapter(std::move(value)) {}
  };

  template <size_t N>
  struct StructuredDataAdapter<char const(&)[N]> : StructuredDataStringAdapter {
    StructuredDataAdapter(const char* str) : StructuredDataStringAdapter(std::string(str, N ? N-1 : 0)) {}
  };

  template <size_t N>
  struct StructuredDataAdapter<char (&)[N]> : StructuredDataStringAdapter {
    StructuredDataAdapter(const char* str) : StructuredDataStringAdapter(std::string(str, N ? N-1 : 0)) {}
  };

  template <size_t N>
  struct StructuredDataAdapter<char[N]> : StructuredDataStringAdapter {
    StructuredDataAdapter(const char* str) : StructuredDataStringAdapter(std::string(str, N ? N-1 : 0)) {}
  };

  template <>
  struct StructuredDataAdapter<const char*> : StructuredDataStringAdapter {
    StructuredDataAdapter(const char* str) : StructuredDataStringAdapter(std::string(str)) {}
  };

  struct StructuredDataIntegerAdapter : StructuredDataValue {
    StructuredDataIntegerAdapter(int64_t n) : number_(n) {}
    NodeType type() const final { return NodeType::Integer; }
    Maybe<int64_t> get_integer() const final { return number_; }
  private:
    int64_t number_;
  };

  // This matches all integer types, and reference/constref to all.
  template <typename T>
  struct StructuredDataAdapter<T,
    typename std::enable_if<
      std::is_integral<T>::value
    >::type
  > : StructuredDataIntegerAdapter {
    StructuredDataAdapter(T number) : StructuredDataIntegerAdapter(number) {}
  };

  struct StructuredDataFloatAdapter : StructuredDataValue {
    StructuredDataFloatAdapter(double n) : number_(n) {}
    NodeType type() const final { return NodeType::Float; }
    Maybe<double> get_float() const final { return number_; }
  private:
    double number_;
  };

  // This matches float, double, and reference/constref to both.
  template <typename T>
  struct StructuredDataAdapter<T,
    typename std::enable_if<
      std::is_floating_point<T>::value
    >::type
  > : StructuredDataFloatAdapter {
    StructuredDataAdapter(T number) : StructuredDataFloatAdapter(number) {}
  };

  template <typename T>
  struct StructuredDataRandomAccessListReferenceAdapter : StructuredDataValue {
    StructuredDataRandomAccessListReferenceAdapter(const T& collection) : collection_(collection) {}
    NodeType type() const final { return NodeType::List; }
    size_t length() const final { return collection_.size(); }
    std::shared_ptr<const IStructuredData> operator[](const std::string&) const final { return nullptr; }
    std::shared_ptr<const IStructuredData> operator[](size_t idx) const final {
      return as_structured_data(collection_.at(idx));
    }
  private:
    const T& collection_;
  };

  template <typename T>
  struct StructuredDataAdapter<std::vector<T>> : StructuredDataRandomAccessListReferenceAdapter<std::vector<T>> {
    using Base = StructuredDataRandomAccessListReferenceAdapter<std::vector<T>>;
    StructuredDataAdapter(std::vector<T>&& collection) : Base(collection_), collection_(std::move(collection)) {}
    StructuredDataAdapter(const std::vector<T>& collection) : Base(collection) {}
    StructuredDataAdapter(std::vector<T>& collection) : Base(collection) {}
  private:
    std::vector<T> collection_;
  };

  template <typename T>
  struct StructuredDataDictionaryReferenceAdapter : StructuredDataValue {
    StructuredDataDictionaryReferenceAdapter(const T& collection) : collection_(collection) {}
    NodeType type() const final { return NodeType::Dictionary; }
    size_t length() const final { return collection_.size(); }
    std::vector<std::string> keys() const {
      std::vector<std::string> k;
      k.reserve(length());
      for (auto& pair: collection_) {
        k.push_back(pair.first);
      }
      return k;
    }
    std::shared_ptr<const IStructuredData> operator[](const std::string& key) const final {
      auto it = collection_.find(key);
      if (it != collection_.end()) {
        return as_structured_data(it->second);
      } else {
        return nullptr;
      }
    }
    std::shared_ptr<const IStructuredData> operator[](size_t idx) const final { return nullptr; }
  private:
    const T& collection_;
  };

  template <typename T>
  struct StructuredDataAdapter<std::map<std::string, T>> : StructuredDataDictionaryReferenceAdapter<std::map<std::string, T>> {
    using Base = StructuredDataDictionaryReferenceAdapter<std::map<std::string, T>>;
    StructuredDataAdapter(std::map<std::string, T>&& collection) : Base(collection_), collection_(std::move(collection)) {}
    StructuredDataAdapter(std::map<std::string, T>& collection) : Base(collection) {}
    StructuredDataAdapter(const std::map<std::string, T>& collection) : Base(collection) {}
  private:
    std::map<std::string, T> collection_;
  };
}

#endif // WAYWARD_SUPPORT_STRUCTURED_DATA_ADAPTERS_HPP_INCLUDED

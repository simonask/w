#pragma once
#ifndef PERSISTENCE_RECORD_AS_STRUCTURED_DATA
#define PERSISTENCE_RECORD_AS_STRUCTURED_DATA

#include <persistence/record_ptr.hpp>
#include <wayward/support/meta.hpp>
#include <wayward/support/structured_data.hpp>
#include <wayward/support/structured_data_adapters.hpp>
#include <wayward/support/datetime.hpp>

namespace persistence {

}

namespace wayward {

  /// Adapter: PrimaryKey
  template <typename T>
  struct StructuredDataAdapter<
    T,
    typename std::enable_if<std::is_same<typename meta::RemoveConstRef<T>::Type, ::persistence::PrimaryKey>::value>::type
  >
  : StructuredDataIntegerAdapter {
    StructuredDataAdapter(::persistence::PrimaryKey key) : StructuredDataIntegerAdapter(key) {}
  };

  /// Adapter: DateTime
  // TODO: Move this to a wayward-library, since DateTime is in Wayward Support.
  template <typename T>
  struct StructuredDataAdapter<
    T,
    typename std::enable_if<std::is_same<typename meta::RemoveConstRef<T>::Type, ::wayward::DateTime>::value>::type
  >
  : StructuredDataValue {
    DateTime dt_;
    StructuredDataAdapter(const DateTime& dt) : dt_(dt) {}
    NodeType type() const override { return NodeType::String; }
    Maybe<std::string> get_string() const override { return dt_.iso8601(); }
  };

  /// Adapter: BelongsTo
  template <typename T>
  struct StructuredDataAdapter<::persistence::BelongsTo<T>>
  : StructuredDataAdapter<::persistence::RecordPtr<T>> {
    StructuredDataAdapter(const ::persistence::BelongsTo<T>& assoc) : StructuredDataAdapter<::persistence::RecordPtr<T>>(assoc.ptr_) {}
  };
  template <typename T>
  struct StructuredDataAdapter<const ::persistence::BelongsTo<T>&>
  : StructuredDataAdapter<::persistence::RecordPtr<T>> {
    StructuredDataAdapter(const ::persistence::BelongsTo<T>& assoc) : StructuredDataAdapter<::persistence::RecordPtr<T>>(assoc.ptr_) {}
  };
  template <typename T>
  struct StructuredDataAdapter<::persistence::BelongsTo<T>&>
  : StructuredDataAdapter<::persistence::RecordPtr<T>> {
    StructuredDataAdapter(const ::persistence::BelongsTo<T>& assoc) : StructuredDataAdapter<::persistence::RecordPtr<T>>(assoc.ptr_) {}
  };

  /// Adapter: RecordPtr<T>
  template <typename T>
  struct StructuredDataAdapter<::persistence::RecordPtr<T>> : IStructuredData {
    ::persistence::RecordPtr<T> ptr_;

    explicit StructuredDataAdapter(::persistence::RecordPtr<T> ptr) : ptr_(std::move(ptr)) {}

    NodeType type() const final {
      return ptr_ ? NodeType::Dictionary : NodeType::Nil;
    }

    size_t length() const final {
      return ::persistence::get_type<T>()->num_properties();
    }

    std::vector<std::string> keys() const final {
      std::vector<std::string> k;
      auto t = ::persistence::get_type<T>();
      size_t n = t->num_properties();
      k.reserve(n);
      for (size_t i = 0; i < n; ++i) {
        auto& property = t->property_at(i);
        k.push_back(property.column());
      }
      return k;
    }

    StructuredDataConstPtr get(const std::string& str) const final {
      if (ptr_ == nullptr) return nullptr;

      auto t = ::persistence::get_type<T>();
      auto property = t->find_property_by_column_name(str);
      if (property) {
        auto property_of = dynamic_cast<const ::persistence::IPropertyOf<T>*>(property);
        if (property_of) {
          return property_of->get_value_as_structured_data(*ptr_);
        }
      }
      return nullptr;
    }

    StructuredDataConstPtr get(size_t idx) const final {
      return nullptr;
    }

    Maybe<std::string>  get_string()  const final { return wayward::Nothing; }
    Maybe<int64_t>      get_integer() const final { return wayward::Nothing; }
    Maybe<double>       get_float()   const final { return wayward::Nothing; }
    Maybe<bool>         get_boolean() const final { return wayward::Nothing; }
  };
}

#endif // PERSISTENCE_RECORD_AS_STRUCTURED_DATA

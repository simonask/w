#pragma once
#ifndef PERSISTENCE_RECORD_AS_STRUCTURED_DATA
#define PERSISTENCE_RECORD_AS_STRUCTURED_DATA

#include <persistence/record_ptr.hpp>
#include <wayward/support/data_franca/adapters.hpp>
#include <wayward/support/datetime.hpp>
#include <wayward/support/monad.hpp>

namespace persistence {

}

namespace wayward {

  namespace data_franca {
    template <typename T>
    struct Adapter<persistence::RecordPtr<T>> : AdapterBase<persistence::RecordPtr<T>> {
      Adapter(persistence::RecordPtr<T>& ref) : AdapterBase<persistence::RecordPtr<T>>(ref) {}

      DataType type() const final { return DataType::Dictionary; }

      bool has_key(const String& key) const final {
        auto t = ::persistence::get_type<T>();
        auto prop = t->find_property_by_column_name(key);
        return prop != nullptr;
      }

      ReaderPtr get(const String& key) const final {
        auto t = ::persistence::get_type<T>();
        auto prop = t->find_property_by_column_name(key);
        if (prop) {
          return prop->get_member_reader(*this->ref_);
        }
        return nullptr;
      }

      size_t length() const final {
        auto t = ::persistence::get_type<T>();
        return t->num_properties();
      }

      AdapterPtr reference_at_key(const String& key) final {
        auto t = ::persistence::get_type<T>();
        auto prop = t->find_property_by_column_name(key);
        if (prop) {
          return prop->get_member_adapter(*this->ref_);
        }
        return nullptr;
      }

      bool set_nothing() final {
        this->ref_ = nullptr;
        return true;
      }

      bool erase(const String& key) final {
        return false; // TODO?
      }

      struct PropertyEnumerator : Cloneable<PropertyEnumerator, IReaderEnumerator> {
        persistence::RecordPtr<T> record;
        const persistence::RecordType<T>* t;
        size_t i = 0;
        PropertyEnumerator(const PropertyEnumerator&) = default;
        PropertyEnumerator(PropertyEnumerator&&) = default;
        PropertyEnumerator(persistence::RecordPtr<T> record) : record(std::move(record)), t(::persistence::get_type<T>()) {}

        ReaderPtr current_value() const final {
          auto prop = t->property_at(i);
          return prop->get_member_reader(*record);
        }

        Maybe<String> current_key() const final {
          auto prop = t->property_at(i);
          return prop->column();
        }

        bool at_end() const final { return i >= t->num_properties(); }
        void move_next() final {
          if (i < t->num_properties()) {
            ++i;
          }
        }
      };

      ReaderEnumeratorPtr enumerator() const final {
        return ReaderEnumeratorPtr{new PropertyEnumerator{this->ref_}};
      }
    };

    template <>
    struct Adapter<persistence::PrimaryKey> : Adapter<int64_t> {
      Adapter(persistence::PrimaryKey& key) : Adapter<int64_t>(key.id) {}
    };

    template <>
    struct Adapter<DateTime> : AdapterBase<DateTime> {
      Adapter(DateTime& ref) : AdapterBase<DateTime>(ref) {}

      // TODO:
      DataType type() const final { return DataType::String; }
      Maybe<std::string> get_string() const final { return this->ref_.iso8601(); }
      bool set_string(String str) final {
        auto dt = DateTime::strptime("%Y-%m-%d %H:%M:%S %z", str);
        if (dt) {
          this->ref_ = *dt;
          return true;
        }
        return false;
      }
    };

    template <typename T>
    struct SingularAssociationAdapter : AdapterBase<T> {
      SingularAssociationAdapter(T& ref) : AdapterBase<T>(ref) {}

      DataType type() const override {
        if (this->ref_.is_populated()) {
          return DataType::Dictionary;
        } else if (this->ref_.is_set()) {
          return DataType::Integer;
        } else {
          return DataType::Nothing;
        }
      }

      Maybe<Integer> get_integer() const override {
        return monad::fmap(this->ref_.id(), [&](const persistence::PrimaryKey& key) { return key.id; });
      }

      bool has_key(const String& key) const override {
        if (type() == DataType::Dictionary) {
          auto& t = this->ref_.foreign_type();
          auto p = t.find_abstract_property_by_column_name(key);
          return p != nullptr;
        }
        return false;
      }

      size_t length() const override {
        if (type() == DataType::Dictionary) {
          return this->ref_.foreign_type().num_properties();
        }
        return 0;
      }

      ReaderPtr get(const String& key) const override {
        if (type() == DataType::Dictionary) {
          auto ptr = this->ref_.get();
          return make_reader(ptr)->get(key);
        }
        return nullptr;
      }

      ReaderEnumeratorPtr enumerator() const override {
        if (type() == DataType::Dictionary) {
          auto ptr = this->ref_.get();
          return make_reader(ptr)->enumerator();
        }
        return nullptr;
      }

      bool set_integer(Integer n) override {
        this->ref_.value_ = persistence::PrimaryKey{n};
        return true;
      }

      AdapterPtr reference_at_key(const String& key) override {
        if (type() == DataType::Dictionary) {
          auto ptr = this->ref_.get();
          return make_adapter(ptr)->reference_at_key(key);
        }
        return nullptr;
      }

      bool erase(const String& key) override {
        if (type() == DataType::Dictionary) {
          auto ptr = this->ref_.get();
          return make_adapter(ptr)->erase(key);
        }
        return false;
      }
    };

    template <typename T>
    struct Adapter<persistence::BelongsTo<T>> : SingularAssociationAdapter<persistence::BelongsTo<T>> {
      Adapter(persistence::BelongsTo<T>& ref) : SingularAssociationAdapter<persistence::BelongsTo<T>>(ref) {}
    };

    template <typename T>
    struct Adapter<persistence::HasOne<T>> : SingularAssociationAdapter<persistence::HasOne<T>> {
      Adapter(persistence::BelongsTo<T>& ref) : SingularAssociationAdapter<persistence::HasOne<T>>(ref) {}
    };

    // template <typename T>
    // struct Adapter<persistence::HasOne<T>> : Adapter<RecordPtr<T>> {
    //   Adapter(persistence::HasOne<T>& ref) : Adapter<RecordPtr<T>>(ref.ptr_) {}
    // };
  }
}

#endif // PERSISTENCE_RECORD_AS_STRUCTURED_DATA

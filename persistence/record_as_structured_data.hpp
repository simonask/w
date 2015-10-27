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
    using ::persistence::get_type;
    using ::persistence::RecordPtr;

    template <class T>
    struct RecordAdapter : AdapterBase<RecordPtr<T>> {
      RecordAdapter(RecordPtr<T>& ref, Bitflags<Options> options) : AdapterBase<RecordPtr<T>>(ref, options) {}

      DataType type() const final {
        return this->ref_ ? DataType::Dictionary : DataType::Nothing;
      }

      bool has_property(const String& key) const {
        return get_type<T>()->find_property_by_column_name(key) != nullptr;
      }

      bool has_association(const String& key) const {
        return get_type<T>()->find_association_by_name(key) != nullptr;
      }

      bool has_key(const String& key) const final {
        return has_property(key) || has_association(key);
      }

      ReaderPtr get(const String& key) const final {
        auto association = get_type<T>()->find_association_by_name(key);
        if (association) {
          return association->get_member_reader(*this->ref_, this->options_);
        }

        auto property = get_type<T>()->find_property_by_column_name(key);
        if (property) {
          return property->get_member_reader(*this->ref_, this->options_);
        }
        return nullptr;
      }

      size_t length() const final {
        return get_type<T>()->num_properties();
      }

      AdapterPtr reference_at_key(const String& key) final {
        auto association = get_type<T>()->find_association_by_name(key);
        if (association) {
          return association->get_member_adapter(*this->ref_, this->options_);
        }

        auto property = get_type<T>()->find_property_by_column_name(key);
        if (property) {
          return property->get_member_adapter(*this->ref_, this->options_);
        }
        return nullptr;
      }

      bool set_nothing() final {
        this->ref_ = nullptr;
        return true;
      }

      bool erase(const String& key) final {
        auto ptr = reference_at_key(key);
        if (ptr) {
          return ptr->set_nothing();
        }
        return false;
      }

      struct PropertyEnumerator : Cloneable<PropertyEnumerator, IReaderEnumerator> {
        persistence::RecordPtr<T> record;
        const persistence::RecordType<T>* t;
        size_t i = 0;
        Bitflags<Options> options_;
        PropertyEnumerator(const PropertyEnumerator&) = default;
        PropertyEnumerator(PropertyEnumerator&&) = default;
        PropertyEnumerator(persistence::RecordPtr<T> record, Bitflags<Options> o) : record(std::move(record)), t(::persistence::get_type<T>()), options_(o) {}

        ReaderPtr current_value() const final {
          auto prop = t->property_at(i);
          return prop->get_member_reader(*record, options_);
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
        return ReaderEnumeratorPtr{new PropertyEnumerator{this->ref_, this->options_}};
      }
    };

    template <typename T>
    struct Adapter<RecordPtr<T>> : RecordAdapter<T> {
      Adapter(RecordPtr<T>& ref, Bitflags<Options> options) : RecordAdapter<T>(ref, options) {}
    };

    template <>
    struct Adapter<persistence::PrimaryKey> : Adapter<int64_t> {
      Adapter(persistence::PrimaryKey& key, Bitflags<Options> o = Options::None) : Adapter<int64_t>(key.id) {}
    };

    template <>
    struct Adapter<DateTime> : AdapterBase<DateTime> {
      Adapter(DateTime& ref, Bitflags<Options> = Options::None) : AdapterBase<DateTime>(ref, Options::None) {}

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
    struct AssociationAdapter : AdapterBase<T> {
      AssociationAdapter(T& ref, Bitflags<Options> o) : AdapterBase<T>(ref, o) {}

      bool can_load() const {
        return this->options_ & Options::AllowLoad;
      }

      bool is_loaded() const {
        return this->ref_.is_loaded();
      }

      void load() const {
        const_cast<T&>(this->ref_).load();
      }
    };

    template <typename T>
    struct SingularAssociationAdapter : AssociationAdapter<T> {
      SingularAssociationAdapter(T& ref, Bitflags<Options> o) : AssociationAdapter<T>(ref, o) {}

      bool has_value() const {
        return this->ref_.is_set();
      }

      DataType type() const override {
        if (this->can_load() || this->is_loaded()) {
          return DataType::Dictionary;
        } else if (has_value()) {
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
          auto t = this->ref_.association()->foreign_type();
          return t->find_abstract_property_by_column_name(key) != nullptr || t->find_abstract_association_by_name(key) != nullptr;
        }
        return false;
      }

      size_t length() const override {
        if (type() == DataType::Dictionary) {
          return this->ref_.association()->foreign_type()->num_properties();
        }
        return 0;
      }

      ReaderPtr value_reader() const {
        if (type() == DataType::Dictionary) {
          if (this->can_load() && !this->is_loaded()) {
            this->load();
          }
          auto ptr = this->ref_.get();
          return make_owning_reader(std::move(ptr), this->options_);
        } else if (has_value()) {
          return make_owning_reader(this->ref_.id());
        }
        return nullptr;
      }

      AdapterPtr value_adapter() const {
        if (type() == DataType::Dictionary) {
          if (this->can_load() && !this->is_loaded()) {
            this->load();
          }
          auto ptr = this->ref_.get();
          return make_owning_adapter(std::move(ptr), this->options_);
        } else if (has_value()) {
          return make_owning_adapter(*this->ref_.id_ptr(), this->options_);
        }
        return nullptr;
      }

      ReaderPtr get(const String& key) const override {
        return value_reader()->get(key);
      }

      ReaderEnumeratorPtr enumerator() const override {
        return value_reader()->enumerator();
      }

      bool set_integer(Integer n) override {
        this->ref_.value_ = persistence::PrimaryKey{n};
        return true;
      }

      AdapterPtr reference_at_key(const String& key) override {
        return value_adapter()->reference_at_key(key);
      }

      bool erase(const String& key) override {
        return value_adapter()->erase(key);
      }
    };

    template <typename T>
    struct PluralAssociationAdapter : AssociationAdapter<T> {
      PluralAssociationAdapter(T& ref, Bitflags<Options> o) : AssociationAdapter<T>(ref, o) {}
      mutable Maybe<ReaderPtr> reader_;

      DataType type() const {
        if (this->can_load() || this->is_loaded()) {
          return DataType::List;
        }
        return DataType::Nothing;
      }

      ReaderPtr value_reader() const {
        if (!reader_ && type() == DataType::List) {
          if (this->can_load() && !this->is_loaded()) {
            this->load();
          }
          reader_ = make_owning_reader(this->ref_.get(), this->options_);
        }
        return *reader_;
      }

      size_t length() const override {
        return value_reader()->length();
      }

      ReaderPtr at(size_t idx) const override {
        return value_reader()->at(idx);
      }

      ReaderEnumeratorPtr enumerator() const override {
        return value_reader()->enumerator();
      }

      AdapterPtr reference_at_index(size_t idx) override {
        return nullptr;
      }
    };

    template <typename T>
    struct Adapter<persistence::BelongsTo<T>> : SingularAssociationAdapter<persistence::BelongsTo<T>> {
      Adapter(persistence::BelongsTo<T>& ref, Bitflags<Options> o) : SingularAssociationAdapter<persistence::BelongsTo<T>>(ref, o) {}
    };

    template <typename T>
    struct Adapter<persistence::HasOne<T>> : SingularAssociationAdapter<persistence::HasOne<T>> {
      Adapter(persistence::HasOne<T>& ref, Bitflags<Options> o) : SingularAssociationAdapter<persistence::HasOne<T>>(ref, o) {}
    };

    template <typename T>
    struct Adapter<persistence::HasMany<T>> : PluralAssociationAdapter<persistence::HasMany<T>> {
      Adapter(persistence::HasMany<T>& ref, Bitflags<Options> o) : PluralAssociationAdapter<persistence::HasMany<T>>(ref, o) {}
    };

    // template <typename T>
    // struct Adapter<persistence::HasOne<T>> : Adapter<RecordPtr<T>> {
    //   Adapter(persistence::HasOne<T>& ref) : Adapter<RecordPtr<T>>(ref.ptr_) {}
    // };
  }
}

#endif // PERSISTENCE_RECORD_AS_STRUCTURED_DATA

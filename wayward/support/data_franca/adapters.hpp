#pragma once
#ifndef WAYWARD_SUPPORT_DATA_FRANCA_ADAPTERS_HPP_INCLUDED
#define WAYWARD_SUPPORT_DATA_FRANCA_ADAPTERS_HPP_INCLUDED

#include <wayward/support/data_franca/adapter.hpp>

namespace wayward {
  namespace data_franca {
    template <>
    struct Adapter<bool> : AdapterBase<bool> {
      Adapter(bool& ref) : AdapterBase<bool>(ref) {}

      DataType type() const final { return DataType::Boolean; }
      Maybe<Boolean> get_boolean() const final { return this->ref_; }
      bool set_boolean(Boolean b) final { this->ref_ = b; return true; }
    };

    template <typename T> struct IsSignedIntegerValue;
    template <> struct IsSignedIntegerValue<signed int> { static const bool Value = true; };
    template <> struct IsSignedIntegerValue<signed char> { static const bool Value = true; };
    template <> struct IsSignedIntegerValue<long long> { static const bool Value = true; };
    template <typename T> struct IsSignedIntegerValue { static const bool Value = false; };

    template <typename T>
    struct Adapter<T, typename std::enable_if<IsSignedIntegerValue<T>::Value>::type> : AdapterBase<T> {
      Adapter(T& ref) : AdapterBase<T>(ref) {}

      DataType type() const final { return DataType::Integer; }
      Maybe<Integer> get_integer() const final { return static_cast<Integer>(this->ref_); }
      bool set_integer(Integer n) final { this->ref_ = static_cast<T>(n); return true; }
    };

    template <typename T>
    struct Adapter<T, typename std::enable_if<std::is_floating_point<T>::value>::type> : AdapterBase<T> {
      Adapter(T& ref) : AdapterBase<T>(ref) {}

      DataType type() const final { return DataType::Real; }
      Maybe<Real> get_real() const final { return static_cast<Real>(this->ref_); }
      bool set_real(Real r) final { this->ref_ = static_cast<T>(r); return true; }
    };

    template <>
    struct Adapter<String> : AdapterBase<String> {
      Adapter(String& ref) : AdapterBase<String>(ref) {}

      DataType type() const final { return DataType::String; }
      Maybe<String> get_string() const final { return this->ref_; }
      bool set_string(String str) final { this->ref_ = std::move(str); return true; }
    };

    template <typename T>
    struct Adapter<std::vector<T>> : AdapterBase<std::vector<T>> {
      Adapter(std::vector<T>& ref) : AdapterBase<std::vector<T>>(ref) {}

      DataType type() const final { return DataType::List; }
      size_t length() const final { return this->ref_.size(); }

      ReaderPtr at(size_t idx) const {
        if (idx < this->ref_.size()) {
          return make_reader(this->ref_.at(idx));
        }
        return nullptr;
      }

      struct Enumerator : IReaderEnumerator {
        using Iterator = typename std::vector<T>::const_iterator;
        Enumerator(Iterator it, Iterator end) : it_(it), end_(end) {}
        Iterator it_;
        Iterator end_;

        ReaderPtr current_value() const final {
          if (it_ != end_) {
            return make_reader(*it_);
          }
          return nullptr;
        }

        Maybe<String> current_key() const final { return Nothing; }

        bool at_end() const final { return it_ == end_; }

        void move_next() final {
          if (!at_end()) {
            ++it_;
          }
        }
      };

      ReaderEnumeratorPtr enumerator() const final {
        return ReaderEnumeratorPtr(new Enumerator{this->ref_.begin(), this->ref_.end()});
      }

      AdapterPtr reference_at_index(size_t idx) final {
        if (idx < this->ref_.size()) {
          return make_adapter(this->ref_.at(idx));
        }
        return nullptr;
      }

      AdapterPtr push_back() final {
        this->ref_.push_back(T{});
        return make_adapter(this->ref_.back());
      }
    };

    template <typename T>
    struct Adapter<std::map<String, T>> : AdapterBase<std::map<String, T>> {
      Adapter(std::map<String, T>& ref) : AdapterBase<std::map<String, T>>(ref) {}

      DataType type() const final { return DataType::Dictionary; }
      size_t length() const final { return this->ref_.size(); }

      bool has_key(const String& key) const final { return this->ref_.find(key) != this->ref_.end(); }

      ReaderPtr get(const String& key) const final {
        auto it = this->ref_.find(key);
        if (it != this->ref_.end()) {
          return make_reader(it->second);
        }
        return nullptr;
      }

      struct Enumerator : IReaderEnumerator {
        using Iterator = typename std::map<String, T>::const_iterator;
        Iterator it_;
        Iterator end_;
        Enumerator(Iterator it, Iterator end) : it_(it), end_(end) {}

        ReaderPtr current_value() const final {
          if (it_ != end_) {
            return make_reader(it_->second);
          }
          return nullptr;
        }

        Maybe<String> current_key() const final {
          if (it_ != end_) {
            return it_->first;
          }
          return Nothing;
        }

        bool at_end() const final { return it_ != end_; }

        void move_next() final {
          if (!at_end()) {
            ++it_;
          }
        }
      };

      ReaderEnumeratorPtr enumerator() const final {
        return ReaderEnumeratorPtr{new Enumerator{this->ref_.begin(), this->ref_.end()}};
      }

      AdapterPtr reference_at_key(const String& key) final {
        auto it = this->ref_.find(key);
        if (it == this->ref_.end()) {
          it = this->ref_.insert(std::make_pair(key, T{})).first;
        }
        return make_adapter(it->second);
      }

      bool erase(const String& key) final {
        auto it = this->ref_.find(key);
        if (it != this->ref_.end()) {
          this->ref_.erase(it);
          return true;
        }
        return false;
      }
    };
  }
}

#endif // WAYWARD_SUPPORT_DATA_FRANCA_ADAPTERS_HPP_INCLUDED

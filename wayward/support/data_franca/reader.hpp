#pragma once
#ifndef WAYWARD_SUPPORT_DATA_FRANCA_READER_HPP_INCLUDED
#define WAYWARD_SUPPORT_DATA_FRANCA_READER_HPP_INCLUDED

#include <wayward/support/data_franca/types.hpp>

#include <wayward/support/maybe.hpp>
#include <wayward/support/cloning_ptr.hpp>

#include <vector>
#include <memory>

namespace wayward {
  namespace data_franca {
    struct IReaderEnumerator;
    using ReaderEnumeratorPtr = CloningPtr<IReaderEnumerator>;

    /*
      A Reader traverses data and inspects it as it passes over it.
      It has the ability to dig down into data structures.
    */
    struct IReader {
      virtual ~IReader() {}

      virtual DataType type() const = 0;

      virtual Maybe<Boolean> get_boolean() const = 0;
      virtual Maybe<Integer> get_integer() const = 0;
      virtual Maybe<Real>    get_real()    const = 0;
      virtual Maybe<String>  get_string()  const = 0;

      virtual bool has_key(const String& key) const = 0;
      virtual ReaderPtr get(const String& key) const = 0;

      virtual size_t   length()       const = 0;
      virtual ReaderPtr at(size_t idx) const = 0;

      virtual ReaderEnumeratorPtr enumerator() const = 0;
    };

    struct IReaderEnumerator {
      virtual ~IReaderEnumerator() {}
      virtual ReaderPtr current_value() const = 0;
      virtual Maybe<String> current_key() const = 0;
      virtual bool at_end() const = 0;
      virtual void move_next() = 0;
      virtual IReaderEnumerator* clone() const = 0;
    };

    struct NullReader final : IReader {
      NullReader() {}
      DataType type() const final { return DataType::Nothing; }
      Maybe<Boolean> get_boolean() const final { return Nothing; }
      Maybe<Integer> get_integer() const final { return Nothing; }
      Maybe<Real>    get_real()    const final { return Nothing; }
      Maybe<String>  get_string()  const final { return Nothing; }
      bool has_key(const String& key) const { return false; }
      std::vector<String> keys() const { return {}; }
      ReaderPtr get(const String& key) const { return nullptr; }
      size_t   length()       const { return 0; }
      ReaderPtr at(size_t idx) const { return nullptr; }
      ReaderEnumeratorPtr enumerator() const { return nullptr; }
    };

    struct ReaderEnumeratorAtEnd : Cloneable<ReaderEnumeratorAtEnd, IReaderEnumerator> {
      ReaderPtr current_value() const final { return nullptr; }
      Maybe<String> current_key() const final { return Nothing; }
      bool at_end() const final { return true; }
      void move_next() final {}
    };

    template <typename Self, typename Subscript = Self>
    struct ReaderInterface {
      bool is_nothing() const;
      operator bool() const;
      bool operator>>(Boolean& b) const;
      bool operator>>(Integer& n) const;
      bool operator>>(Real& r) const;
      bool operator>>(String& str) const;
      Subscript reader_subscript(size_t idx) const;
      Subscript reader_subscript(const String& key) const;
      bool has_key(const String& key) const;
      size_t length() const;
      struct iterator;
      iterator begin() const;
      iterator end()   const;

    protected:
      ReaderInterface() {}
    private:
      // Could be an IReader, could be something else...
      template <typename Self_ = Self>
      auto reader() const -> decltype(std::declval<const Self_>().reader_iface()) {
        return static_cast<const Self_*>(this)->reader_iface();
      }
    };

    template <typename Self, typename Subscript>
    struct ReaderInterface<Self, Subscript>::iterator {
      iterator(const iterator&) = default;
      iterator(iterator&&) = default;

      bool operator==(const iterator& other) const {
        if (enumerator_ == nullptr) {
          if (other.enumerator_ == nullptr) {
            return true;
          } else {
            return other.enumerator_->at_end();
          }
        } else {
          if (other.enumerator_ == nullptr) {
            return enumerator_->at_end();
          } else {
            return enumerator_->at_end() == other.enumerator_->at_end();
          }
        }
      }

      bool operator!=(const iterator& other) const { return !(*this == other); }
      const Subscript& operator*() const { return current_; }
      const Subscript* operator->() const { return &current_; }

      Maybe<String> key() const { return enumerator_->current_key(); }

      iterator& operator++() { enumerator_->move_next(); update_ptr(); return *this; }
    private:
      iterator(ReaderEnumeratorPtr e) : enumerator_{std::move(e)} { update_ptr(); }
      ReaderEnumeratorPtr enumerator_;
      Subscript current_;
      friend struct ReaderInterface<Self, Subscript>;
      void update_ptr() {
        if (enumerator_ && !enumerator_->at_end()) {
          auto c = enumerator_->current_value();
          current_ = std::move(c);
        }
      }
    };

    template <typename Self, typename Subscript>
    bool ReaderInterface<Self, Subscript>::is_nothing() const {
      return reader().type() == DataType::Nothing;
    }

    template <typename Self, typename Subscript>
    ReaderInterface<Self, Subscript>::operator bool() const {
      return !is_nothing();
    }

    template <typename Self, typename Subscript>
    bool ReaderInterface<Self, Subscript>::operator>>(Boolean& b) const {
      if (reader().type() == DataType::Boolean) {
        b = *reader().get_boolean();
        return true;
      }
      return false;
    }

    template <typename Self, typename Subscript>
    bool ReaderInterface<Self, Subscript>::operator>>(Integer& n) const {
      auto type = reader().type();
      switch (type) {
        case DataType::Integer: {
          n = *reader().get_integer();
          return true;
        }
        case DataType::Real: {
          Real r = *reader().get_real();
          n = static_cast<Integer>(r);
          return true;
        }
        case DataType::String: {
          std::stringstream ss(*reader().get_string());
          return (ss >> n).eof();
        }
        default:
          return false;
      }
    }

    template <typename Self, typename Subscript>
    bool ReaderInterface<Self, Subscript>::operator>>(Real& r) const {
      auto type = reader().type();
      switch (type) {
        case DataType::Integer: {
          Integer n = *reader().get_integer();
          r = static_cast<Real>(n);
          return true;
        }
        case DataType::Real: {
          r = *reader().get_real();
          return true;
        }
        case DataType::String: {
          std::stringstream ss(*reader().get_string());
          return (ss >> r).eof();
        }
        default:
          return false;
      }
    }

    template <typename Self, typename Subscript>
    bool ReaderInterface<Self, Subscript>::operator>>(String& str) const {
      auto type = reader().type();
      switch (type) {
        case DataType::Integer: {
          std::stringstream ss;
          ss << *reader().get_integer();
          str = ss.str();
          return true;
        }
        case DataType::Real: {
          std::stringstream ss;
          ss << *reader().get_real();
          str = ss.str();
          return true;
        }
        case DataType::String: {
          str = *reader().get_string();
          return true;
        }
        default:
          return false;
      }
    }

    template <typename Self, typename Subscript>
    typename ReaderInterface<Self, Subscript>::iterator
    ReaderInterface<Self, Subscript>::begin() const {
      return iterator{reader().enumerator()};
    }

    template <typename Self, typename Subscript>
    typename ReaderInterface<Self, Subscript>::iterator
    ReaderInterface<Self, Subscript>::end() const {
      return iterator{ReaderEnumeratorPtr{new ReaderEnumeratorAtEnd}};
    }

    template <typename Self, typename Subscript>
    size_t ReaderInterface<Self, Subscript>::length() const {
      return reader().length();
    }

    template <typename Self, typename Subscript>
    Subscript ReaderInterface<Self, Subscript>::reader_subscript(size_t idx) const {
      return reader().at(idx);
    }

    template <typename Self, typename Subscript>
    Subscript ReaderInterface<Self, Subscript>::reader_subscript(const String& key) const {
      return reader().get(key);
    }

    template <typename Self, typename Subscript>
    bool ReaderInterface<Self, Subscript>::has_key(const String& key) const {
      return reader().has_key(key);
    }
  }
}

#endif // WAYWARD_SUPPORT_DATA_FRANCA_READER_HPP_INCLUDED

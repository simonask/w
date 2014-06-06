#pragma once
#ifndef WAYWARD_SUPPORT_DATA_FRANCA_OBJECT_HPP_INCLUDED
#define WAYWARD_SUPPORT_DATA_FRANCA_OBJECT_HPP_INCLUDED

#include <wayward/support/either.hpp>
#include <wayward/support/error.hpp>

#include <wayward/support/data_franca/reader.hpp>
#include <wayward/support/data_franca/writer.hpp>

#include <wayward/support/cloning_ptr.hpp>

namespace wayward {
  namespace data_franca {
    /*
      An "object" is a piece of structured data that can be modified.

      Think JSON builder.
    */
    struct Object final
    : ReaderInterface<Object, const Object&>
    , WriterInterface<Object, Object&> {
      Object(NothingType = Nothing) : data_(Nothing) {}
      Object(Boolean b) : data_(b) {}
      Object(Integer n) : data_(n) {}
      Object(Real r)    : data_(r) {}
      Object(String s)  : data_(std::move(s)) {}

      // Convenience:
      Object(int n) : data_((Integer)n) {}
      Object(const char* str) : data_(std::string{str}) {}

      Object* clone() const { return new Object(*this); }

      DataType type() const;
      Object& operator[](size_t idx) { return this->writer_subscript(idx); }
      const Object& operator[](size_t idx) const { return this->reader_subscript(idx); }
      Object& operator[](const String& key) { return this->writer_subscript(key); }
      const Object& operator[](const String& key) const { return this->reader_subscript(key); }

      // This sets the type to 'List':
      void reserve(size_t idx);

      // ReaderInterface required interface:
      const Object& reader_iface() const { return *this; }
      Maybe<Boolean> get_boolean() const { return data_.get<Boolean>(); }
      Maybe<Integer> get_integer() const { return data_.get<Integer>(); }
      Maybe<Real>    get_real() const    { return data_.get<Real>(); }
      Maybe<String>  get_string() const  { return data_.get<String>(); }
      bool           has_key(const String& key) const;
      const Object&  get(const String& key) const;
      size_t         length() const;
      const Object&  at(size_t idx) const;

      // WriterInterface required interface:
      Object& writer_iface() { return *this; }
      bool set_boolean(Boolean b) { data_ = b; return true; }
      bool set_integer(Integer n) { data_ = n; return true; }
      bool set_real(Real r) { data_ = r; return true; }
      bool set_string(String str) { data_ = std::move(str); return true; }
      Object& reference_at_index(size_t idx);
      bool push_back(Object other);
      Object& reference_at_key(const String& key);
      bool erase(const String& key);

    private:
      using List = std::vector<CloningPtr<Object>>;
      using Dictionary = std::map<std::string, CloningPtr<Object>>;

      Either<
        NothingType,
        Boolean,
        Integer,
        Real,
        String,
        List,
        Dictionary
      > data_;

      static const Object g_null_object;
    };

    inline DataType Object::type() const {
      // XXX: Slightly fragile, but never change the order of the Either.
      switch (data_.which()) {
        case 0: return DataType::Nothing;
        case 1: return DataType::Boolean;
        case 2: return DataType::Integer;
        case 3: return DataType::Real;
        case 4: return DataType::String;
        case 5: return DataType::List;
        case 6: return DataType::Dictionary;
        default: return DataType::Nothing;
      }
    }

    struct ObjectIndexOutOfBounds : Error {
      ObjectIndexOutOfBounds(const std::string& msg) : Error{msg} {}
    };

    inline Object& Object::reference_at_index(size_t idx) {
      if (type() != DataType::List) {
        throw ObjectIndexOutOfBounds{"Object is not a list."};
      }
      Object* ptr = nullptr;
      data_.template when<List>([&](List& list) {
        if (idx < list.size()) {
          ptr = list[idx].get();
        }
      });
      if (ptr == nullptr) {
        throw ObjectIndexOutOfBounds{"Index out of bounds."};
      }
      return *ptr;
    }

    inline const Object& Object::at(size_t idx) const {
      if (type() != DataType::List) {
        throw ObjectIndexOutOfBounds{"Object is not a list."};
      }
      const Object* ptr = nullptr;
      data_.template when<List>([&](const List& list) {
        if (idx < list.size()) {
          ptr = list[idx].get();
        }
      });
      if (ptr == nullptr) {
        throw ObjectIndexOutOfBounds{"Index out of bounds."};
      }
      return *ptr;
    }

    inline Object& Object::reference_at_key(const String& key) {
      if (type() != DataType::Dictionary) {
        data_ = Dictionary{};
      }
      Object* ptr = nullptr;
      data_.template when<Dictionary>([&](Dictionary& dict) {
        auto it = dict.find(key);
        if (it == dict.end()) {
          it = dict.insert(std::make_pair(key, CloningPtr<Object>{new Object})).first;
        }
        ptr = it->second.get();
      });
      assert(ptr != nullptr);
      return *ptr;
    }

    inline bool Object::push_back(Object other) {
      if (type() != DataType::List) {
        data_ = List{};
      }
      data_.template when<List>([&](List& list) {
        list.emplace_back(new Object(std::move(other)));
      });
      return true;
    }

    inline size_t Object::length() const {
      size_t len = 0;
      data_.template when<List>([&](const List& list) {
        len = list.size();
      });
      data_.template when<Dictionary>([&](const Dictionary& dict) {
        len = dict.size();
      });
      return len;
    }
  }
}

#endif // WAYWARD_SUPPORT_DATA_FRANCA_OBJECT_HPP_INCLUDED

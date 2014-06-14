#pragma once
#ifndef WAYWARD_SUPPORT_DATA_FRANCA_OBJECT_HPP_INCLUDED
#define WAYWARD_SUPPORT_DATA_FRANCA_OBJECT_HPP_INCLUDED

#include <wayward/support/either.hpp>
#include <wayward/support/error.hpp>

#include <wayward/support/data_franca/reader.hpp>
#include <wayward/support/data_franca/writer.hpp>
#include <wayward/support/data_franca/adapter.hpp>

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

      Object(const Object&) = default;
      Object(Object&&) = default;
      Object& operator=(Object&&) = default;
      Object& operator=(const Object&) = default;


      // Convenience:
      Object(int n) : data_((Integer)n) {}
      Object(const char* str) : data_(std::string{str}) {}
      static Object dictionary() { Object o; o.data_ = Dictionary{}; return std::move(o); }
      static Object list() { Object o; o.data_ = List{}; return std::move(o); }

      Object* clone() const { return new Object(*this); }

      DataType type() const;
      Object& operator[](size_t idx) { return this->writer_subscript(idx); }
      const Object& operator[](size_t idx) const { return this->reader_subscript(idx); }
      Object& operator[](const String& key) { return this->writer_subscript(key); }
      const Object& operator[](const String& key) const { return this->reader_subscript(key); }
      Object& operator[](const char* key) { return this->writer_subscript(key); }
      const Object& operator[](const char* key) const { return this->reader_subscript(key); }

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

      struct ListEnumerator;
      struct DictEnumerator;
      ReaderEnumeratorPtr enumerator() const;

      // WriterInterface required interface:
      Object& writer_iface() { return *this; }
      bool set_nothing() { data_ = Nothing; return true; }
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

      friend struct Enumerator;
      friend struct Adapter<Object>;
    };

    struct ObjectIndexOutOfBounds : Error {
      ObjectIndexOutOfBounds(const std::string& msg) : Error{msg} {}
    };

    template <>
    struct Adapter<Object> : IAdapter {
      Adapter(Object& ref, Bitflags<Options> options) : ref_(ref), options_(options) {}

      // IReader interface:
      DataType type() const final { return ref_.type(); }
      Maybe<Boolean> get_boolean() const final { return ref_.get_boolean(); }
      Maybe<Integer> get_integer() const final { return ref_.get_integer(); }
      Maybe<Real>    get_real()    const final { return ref_.get_real(); }
      Maybe<String>  get_string()  const final { return ref_.get_string(); }
      bool has_key(const String& key) const final { return ref_.has_key(key); }
      ReaderPtr get(const String& key) const final { return make_reader(ref_.get(key), options_); }
      size_t   length()       const final { return ref_.length(); }
      ReaderPtr at(size_t idx) const final { return make_reader(ref_.at(idx), options_); }
      ReaderEnumeratorPtr enumerator() const final { return ref_.enumerator(); }

      // IWriter interface:
      bool set_nothing() final { return ref_.set_nothing(); }
      bool set_boolean(Boolean b) final { return ref_.set_boolean(b); }
      bool set_integer(Integer n) final { return ref_.set_integer(n); }
      bool set_real(Real r) final { return ref_.set_real(r); }
      bool set_string(String str) final { return ref_.set_string(std::move(str)); }
      AdapterPtr reference_at_index(size_t idx) final { return make_adapter(ref_.reference_at_index(idx), options_); }
      AdapterPtr push_back() final {
        ref_.push_back(Object{});
        AdapterPtr ptr;
        ref_.data_.template when<Object::List>([&](Object::List& list) {
          ptr = make_adapter(*list.back(), options_);
        });
        return std::move(ptr);
      }
      AdapterPtr reference_at_key(const String& key) final { return make_adapter(ref_.reference_at_key(key), options_); }
      bool erase(const String& key) final { return ref_.erase(key); }

      Object& ref_;
      Bitflags<Options> options_;
    };
  }
}

#endif // WAYWARD_SUPPORT_DATA_FRANCA_OBJECT_HPP_INCLUDED

#pragma once
#ifndef WAYWARD_SUPPORT_DATA_FRANCA_OBJECT_HPP_INCLUDED
#define WAYWARD_SUPPORT_DATA_FRANCA_OBJECT_HPP_INCLUDED

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

      DataType type() const;

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
  }
}

#endif // WAYWARD_SUPPORT_DATA_FRANCA_OBJECT_HPP_INCLUDED

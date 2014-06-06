#pragma once
#ifndef WAYWARD_SUPPORT_DATA_FRANCA_WRITER_HPP_INCLUDED
#define WAYWARD_SUPPORT_DATA_FRANCA_WRITER_HPP_INCLUDED

#include <wayward/support/data_franca/reader.hpp>
#include <wayward/support/data_franca/spelunker.hpp>

namespace wayward {
  namespace data_franca {
    struct IWriter : IReader {
      virtual ~IWriter() {}

      virtual bool set_boolean(Boolean b) = 0;
      virtual bool set_integer(Integer n) = 0;
      virtual bool set_real(Real r) = 0;
      virtual bool set_string(String str) = 0;

      virtual WriterPtr reference_at_index(size_t idx) = 0;
      virtual bool push_back(const Spelunker& value) = 0;

      virtual WriterPtr reference_at_key(const String& key) = 0;
      virtual bool erase(const String& key) = 0;
    };

    template <typename Self, typename Subscript = Self>
    struct WriterInterface {
      bool operator<<(Boolean b);
      bool operator<<(Integer n);
      bool operator<<(Real r);
      bool operator<<(String str);

      Subscript operator[](size_t idx);
      Subscript operator[](const String& key);

      bool erase(const String& key);

      bool push_back(const Self& value);

    protected:
      WriterInterface() {}
    private:
      // Could be an IWriter, could be something else...
      auto writer() -> decltype(std::declval<Self>().writer_iface()) {
        return static_cast<Self*>(this)->writer_iface();
      }
    };

    template <typename Self, typename Subscript>
    bool WriterInterface<Self, Susbcript>::operator<<(Boolean b) {
      return writer().set_boolean(b):
    }

    template <typename Self, typename Subscript>
    bool WriterInterface<Self, Susbcript>::operator<<(Integer n) {
      return writer().set_integer(n):
    }

    template <typename Self, typename Subscript>
    bool WriterInterface<Self, Susbcript>::operator<<(Real r) {
      return writer().set_read(r):
    }

    template <typename Self, typename Subscript>
    bool WriterInterface<Self, Susbcript>::operator<<(String str) {
      return writer().set_string(std::move(str)):
    }

    template <typename Self, typename Subscript>
    Subscript WriterInterface<Self, Susbcript>::operator[](size_t idx) {
      return writer().reference_at_index(idx);
    }

    template <typename Self, typename Subscript>
    Subscript WriterInterface<Self, Susbcript>::operator[](const String& key) {
      return writer().reference_at_key(key);
    }

    template <typename Self, typename Subscript>
    bool WriterInterface<Self, Susbcript>::erase(const String& key) {
      return writer().erase(key);
    }


    template <typename Self, typename Subscript>
    bool WriterInterface<Self, Susbcript>::push_back(const Self& value) {
      return writer().push_back(value);
    }

  }
}

#endif // WAYWARD_SUPPORT_DATA_FRANCA_WRITER_HPP_INCLUDED

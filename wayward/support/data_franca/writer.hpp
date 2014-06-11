#pragma once
#ifndef WAYWARD_SUPPORT_DATA_FRANCA_WRITER_HPP_INCLUDED
#define WAYWARD_SUPPORT_DATA_FRANCA_WRITER_HPP_INCLUDED

#include <wayward/support/data_franca/reader.hpp>
#include <wayward/support/data_franca/types.hpp>

namespace wayward {
  namespace data_franca {
    struct Spectator;

    struct IWriter {
      virtual ~IWriter() {}

      virtual bool set_nothing() = 0;
      virtual bool set_boolean(Boolean b) = 0;
      virtual bool set_integer(Integer n) = 0;
      virtual bool set_real(Real r) = 0;
      virtual bool set_string(String str) = 0;

      virtual AdapterPtr reference_at_index(size_t idx) = 0;
      virtual AdapterPtr push_back() = 0;

      virtual AdapterPtr reference_at_key(const String& key) = 0;
      virtual bool erase(const String& key) = 0;
    };

    template <typename Self, typename Subscript = Self>
    struct WriterInterface {
      bool operator<<(NothingType);
      bool operator<<(Boolean b);
      bool operator<<(Integer n);
      bool operator<<(Real r);
      bool operator<<(String str);

      // Convenience:
      bool operator<<(int n) { return *this << (Integer)n; }
      bool operator<<(const char* str) { return *this << std::string{str}; }

      Subscript writer_subscript(size_t idx);
      Subscript writer_subscript(const String& key);

      bool erase(const String& key);

      template <typename T>
      bool push_back(const T& value);

    protected:
      WriterInterface() {}
    private:
      // Could be an IWriter, could be something else...
      template <typename Self_ = Self>
      auto writer() -> decltype(std::declval<Self_>().writer_iface()) {
        return static_cast<Self_*>(this)->writer_iface();
      }
    };

    template <typename Self, typename Subscript>
    bool WriterInterface<Self, Subscript>::operator<<(NothingType) {
      return writer().set_nothing();
    }

    template <typename Self, typename Subscript>
    bool WriterInterface<Self, Subscript>::operator<<(Boolean b) {
      return writer().set_boolean(b);
    }

    template <typename Self, typename Subscript>
    bool WriterInterface<Self, Subscript>::operator<<(Integer n) {
      return writer().set_integer(n);
    }

    template <typename Self, typename Subscript>
    bool WriterInterface<Self, Subscript>::operator<<(Real r) {
      return writer().set_real(r);
    }

    template <typename Self, typename Subscript>
    bool WriterInterface<Self, Subscript>::operator<<(String str) {
      return writer().set_string(std::move(str));
    }

    template <typename Self, typename Subscript>
    Subscript WriterInterface<Self, Subscript>::writer_subscript(size_t idx) {
      return writer().reference_at_index(idx);
    }

    template <typename Self, typename Subscript>
    Subscript WriterInterface<Self, Subscript>::writer_subscript(const String& key) {
      return writer().reference_at_key(key);
    }

    template <typename Self, typename Subscript>
    bool WriterInterface<Self, Subscript>::erase(const String& key) {
      return writer().erase(key);
    }


    template <typename Self, typename Subscript>
    template <typename T>
    bool WriterInterface<Self, Subscript>::push_back(const T& value) {
      auto pushed = writer().push_back();
      return Self{pushed} << value;
    }

  }
}

#endif // WAYWARD_SUPPORT_DATA_FRANCA_WRITER_HPP_INCLUDED

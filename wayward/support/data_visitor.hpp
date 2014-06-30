#pragma once
#ifndef WAYWARD_SUPPORT_DATA_VISITOR_HPP_INCLUDED
#define WAYWARD_SUPPORT_DATA_VISITOR_HPP_INCLUDED

#include <wayward/support/type.hpp>

namespace wayward {
  struct DataVisitor {
    // Implement this interface:

    virtual void visit_nil() = 0;
    virtual void visit_boolean(bool&) = 0;
    virtual void visit_int8(std::int8_t&) = 0;
    virtual void visit_int16(std::int16_t&) = 0;
    virtual void visit_int32(std::int32_t&) = 0;
    virtual void visit_int64(std::int64_t&) = 0;
    virtual void visit_uint8(std::uint8_t&) = 0;
    virtual void visit_uint16(std::uint16_t&) = 0;
    virtual void visit_uint32(std::uint32_t&) = 0;
    virtual void visit_uint64(std::uint64_t&) = 0;
    virtual void visit_float(float&) = 0;
    virtual void visit_double(double&) = 0;
    virtual void visit_string(std::string&) = 0;
    virtual void visit_key_value(const std::string& key, AnyRef data, const IType* type) = 0;
    virtual void visit_element(std::int64_t idx, AnyRef data, const IType* type) = 0;
    virtual void visit_special(AnyRef data, const IType* type) = 0;
    virtual bool can_modify() const = 0;
    virtual bool is_nil_at_current() const = 0;


    // Specialize this if your data type can be visited as one of the normal methods listed above.
    // If it isn't specialized, visit_special will be called.
    template <class T> struct VisitCaller;

    // Call this interface from types:

    template <class T>
    void visit(T& value) {
      using Type = typename meta::RemoveConstRef<T>::Type;
      VisitCaller<Type>::visit(*this, value);
    }

    template <class T>
    void operator()(T& value) {
      visit(value);
    }

    template <class T>
    void visit(const std::string& key, T& value) {
      using Type = typename meta::RemoveConstRef<T>::Type;
      this->visit_key_value(key, AnyRef{value}, get_type<Type>());
    }

    struct KeyValueVisitor {
      DataVisitor& visitor;
      std::string key;
      KeyValueVisitor(DataVisitor& visitor, std::string key) : visitor(visitor), key(std::move(key)) {}
      template <class T>
      void operator()(T& value) {
        visitor.visit(key, value);
      }
    };

    KeyValueVisitor operator[](const std::string& key) {
      return KeyValueVisitor{*this, key};
    }
    KeyValueVisitor operator[](const char* key) {
      return KeyValueVisitor{*this, key};
    }

    template <class T>
    void visit(std::int64_t idx, T& value) {
      using Type = typename meta::RemoveConstRef<T>::Type;
      this->visit_element(idx, AnyRef{value}, get_type<Type>());
    }

    struct ElementVisitor {
      DataVisitor& visitor;
      std::int64_t index;
      ElementVisitor(DataVisitor& visitor, std::int64_t index) : visitor(visitor), index(index) {}
      template <class T>
      void operator()(T& value) {
        visitor.visit(index, value);
      }
    };

    ElementVisitor operator[](std::int64_t index) {
      return ElementVisitor{*this, index};
    }
  };

  template <> struct DataVisitor::VisitCaller<NothingType>   { static void visit(DataVisitor& visitor, NothingType) { visitor.visit_nil(); } };
  template <> struct DataVisitor::VisitCaller<bool>          { static void visit(DataVisitor& visitor, bool& value) { visitor.visit_boolean(value); } };
  template <> struct DataVisitor::VisitCaller<std::int8_t>   { static void visit(DataVisitor& visitor, std::int8_t& value) { visitor.visit_int8(value); } };
  template <> struct DataVisitor::VisitCaller<std::int16_t>  { static void visit(DataVisitor& visitor, std::int16_t& value) { visitor.visit_int16(value); } };
  template <> struct DataVisitor::VisitCaller<std::int32_t>  { static void visit(DataVisitor& visitor, std::int32_t& value) { visitor.visit_int32(value); } };
  template <> struct DataVisitor::VisitCaller<std::int64_t>  { static void visit(DataVisitor& visitor, std::int64_t& value) { visitor.visit_int64(value); } };
  template <> struct DataVisitor::VisitCaller<std::uint8_t>  { static void visit(DataVisitor& visitor, std::uint8_t& value) { visitor.visit_uint8(value); } };
  template <> struct DataVisitor::VisitCaller<std::uint16_t> { static void visit(DataVisitor& visitor, std::uint16_t& value) { visitor.visit_uint16(value); } };
  template <> struct DataVisitor::VisitCaller<std::uint32_t> { static void visit(DataVisitor& visitor, std::uint32_t& value) { visitor.visit_uint32(value); } };
  template <> struct DataVisitor::VisitCaller<std::uint64_t> { static void visit(DataVisitor& visitor, std::uint64_t& value) { visitor.visit_uint64(value); } };
  template <> struct DataVisitor::VisitCaller<float>         { static void visit(DataVisitor& visitor, float& value) { visitor.visit_float(value); } };
  template <> struct DataVisitor::VisitCaller<double>        { static void visit(DataVisitor& visitor, double& value) { visitor.visit_double(value); } };
  template <> struct DataVisitor::VisitCaller<std::string>   { static void visit(DataVisitor& visitor, std::string& value) { visitor.visit_string(value); } };

  template <class T> struct DataVisitor::VisitCaller {
    using Type = typename meta::RemoveConstRef<T>::Type;
    static void visit(DataVisitor& visitor, T& value) {
      visitor.visit_special(value, get_type<T>());
    }
  };
}

#endif // WAYWARD_SUPPORT_DATA_VISITOR_HPP_INCLUDED

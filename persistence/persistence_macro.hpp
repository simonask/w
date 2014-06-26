#pragma once
#ifndef PERSISTENCE_PERSISTENCE_MACRO_HPP_INCLUDED
#define PERSISTENCE_PERSISTENCE_MACRO_HPP_INCLUDED

#include <persistence/record_type_builder.hpp>

#define PERSISTENCE(TYPE) \
  struct RecordTypeBuilder_ ## TYPE : persistence::RecordTypeBuilder<TYPE> { \
    void build_(); \
    void build_with_defaults_() { \
      name(#TYPE); \
      std::string default_relation_name = #TYPE; \
      std::transform(default_relation_name.begin(), default_relation_name.end(), default_relation_name.begin(), ::tolower); \
      default_relation_name += "s"; \
      relation(default_relation_name); \
      build_(); \
    } \
  }; \
  inline const ::persistence::RecordType<TYPE>* build_type(const ::wayward::TypeIdentifier<TYPE>* dummy) { \
    RecordTypeBuilder_ ## TYPE builder; \
    builder.type_ = new ::persistence::RecordType<TYPE>; \
    builder.build_with_defaults_(); \
    return builder.result_(); \
  } \
  inline void RecordTypeBuilder_ ## TYPE ::build_()

#endif // PERSISTENCE_PERSISTENCE_MACRO_HPP_INCLUDED

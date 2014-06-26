#pragma once
#ifndef PERSISTENCE_SQL_TYPE_MAPPER_HPP_INCLUDED
#define PERSISTENCE_SQL_TYPE_MAPPER_HPP_INCLUDED

namespace wayward {
  struct AnyConstRef;
  template <typename> struct CloningPtr;
}

namespace persistence {
  namespace ast {
    struct SingleValue;
  }
  using wayward::AnyConstRef;

  struct ISQLTypeMapper {
    virtual wayward::CloningPtr<ast::SingleValue> literal_for_value(AnyConstRef value) = 0;
  };
}

#endif // PERSISTENCE_SQL_TYPE_MAPPER_HPP_INCLUDED

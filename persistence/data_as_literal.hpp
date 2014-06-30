#pragma once
#ifndef PERSISTENCE_DATA_AS_LITERAL_HPP_INCLUDED
#define PERSISTENCE_DATA_AS_LITERAL_HPP_INCLUDED

#include <wayward/support/data_visitor.hpp>
#include <wayward/support/any.hpp>

#include <persistence/ast.hpp>

namespace persistence {
  using wayward::AnyRef;
  using wayward::IType;

  struct DataAsLiteral {
    struct Visitor;

    template <class T>
    ast::Ptr<ast::SingleValue> make_literal(T& data) {
      return make_literal(data, wayward::get_type<T>());
    }

    ast::Ptr<ast::SingleValue> make_literal(AnyRef, const IType*);
  private:
    ast::Ptr<ast::SingleValue> result;
  };
}

#endif // PERSISTENCE_DATA_AS_LITERAL_HPP_INCLUDED

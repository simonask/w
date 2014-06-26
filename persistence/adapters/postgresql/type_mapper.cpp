#include "persistence/adapters/postgresql/type_mapper.hpp"
#include "persistence/adapters/postgresql/connection.hpp"
#include "persistence/ast.hpp"

#include <wayward/support/cloning_ptr.hpp>
#include <wayward/support/any.hpp>
#include <wayward/support/datetime.hpp>

namespace persistence {
  namespace {
    template <typename Lit, typename... Args>
    wayward::CloningPtr<ast::SingleValue>
    make_literal(Args&&... args) {
      return wayward::CloningPtr<ast::SingleValue>{ new Lit{ std::forward<Args>(args)... } };
    }
  }

  wayward::CloningPtr<ast::SingleValue>
  PostgreSQLTypeMapper::literal_for_value(AnyConstRef data) {
    if (data.is_a<wayward::NothingType>()) {
      return make_literal<ast::SQLFragmentValue>("NULL");
    } else
    if (data.is_a<int>()) {
      return make_literal<ast::NumericLiteral>((double)*data.get<int>());
    } else
    if (data.is_a<int64_t>()) {
      return make_literal<ast::NumericLiteral>((double)*data.get<int64_t>());
    } else
    if (data.is_a<float>()) {
      return make_literal<ast::NumericLiteral>((float)*data.get<float>());
    } else
    if (data.is_a<double>()) {
      return make_literal<ast::NumericLiteral>(*data.get<double>());
    } else
    if (data.is_a<std::string>()) {
      return make_literal<ast::StringLiteral>(*data.get<std::string>());
    } else
    if (data.is_a<bool>()) {
      return make_literal<ast::BooleanLiteral>(*data.get<bool>());
    } else
    if (data.is_a<wayward::DateTime>()) {
      // TODO:
      return make_literal<ast::StringLiteral>(data.get<wayward::DateTime>()->iso8601());
    }
    throw PostgreSQLError{ wayward::format("Unsupported type: {0}", data.type_info().name()) };
  }
}

#include "persistence/data_as_literal.hpp"

#include <wayward/support/data_visitor.hpp>
#include <wayward/support/datetime.hpp>

namespace persistence {
  using namespace ast;
  using wayward::TypeError;
  using wayward::DateTime;

  namespace {
    template <class T, class... Args>
    ast::Ptr<SingleValue> make_ast_literal(Args&&... args) {
      return ast::Ptr<SingleValue>{ new T{std::forward<Args>(args)...} };
    }
  }

  struct DataAsLiteral::Visitor : wayward::DataVisitor {
    DataAsLiteral& p;
    Visitor(DataAsLiteral& owner) : p(owner) {}

    void visit_nil() {
      p.result = make_ast_literal<SQLFragmentValue>("NULL");
    }

    void visit_boolean(bool& value) {
      p.result = make_ast_literal<BooleanLiteral>(value);
    }

    void visit_int8(std::int8_t& value) {
      p.result = make_ast_literal<NumericLiteral>((double)value);
    }

    void visit_int16(std::int16_t& value) {
      p.result = make_ast_literal<NumericLiteral>((double)value);
    }

    void visit_int32(std::int32_t& value) {
      p.result = make_ast_literal<NumericLiteral>((double)value);
    }

    void visit_int64(std::int64_t& value) {
      p.result = make_ast_literal<NumericLiteral>((double)value);
    }

    void visit_uint8(std::uint8_t& value) {
      p.result = make_ast_literal<NumericLiteral>((double)value);
    }

    void visit_uint16(std::uint16_t& value) {
      p.result = make_ast_literal<NumericLiteral>((double)value);
    }

    void visit_uint32(std::uint32_t& value) {
      p.result = make_ast_literal<NumericLiteral>((double)value);
    }

    void visit_uint64(std::uint64_t& value) {
      p.result = make_ast_literal<NumericLiteral>((double)value);
    }

    void visit_float(float& value) {
      p.result = make_ast_literal<NumericLiteral>((double)value);
    }

    void visit_double(double& value) {
      p.result = make_ast_literal<NumericLiteral>((double)value);
    }

    void visit_string(std::string& value) {
      p.result = make_ast_literal<StringLiteral>(value);
    }

    void visit_key_value(const std::string& key, AnyRef data, const IType* type) {
      throw TypeError{"Cannot represent key-value data type as SQL literal."};
    }

    void visit_element(std::int64_t idx, AnyRef data, const IType* type) {
      if (idx < 0) {
        throw TypeError{"Negative list element index has no meaning in SQL."};
      }
      auto q = dynamic_cast<ast::List*>(p.result.get());
      if (!q) {
        q = new List;
        p.result = ast::Ptr<ast::SingleValue>{ q };
      }
      q->elements.reserve(idx);
      if (q->elements.size() <= idx) {
        q->elements.resize(idx+1);
      }
      DataAsLiteral sub;
      q->elements[idx] = sub.make_literal(data, type);
    }

    void visit_special(AnyRef data, const IType* type) {
      if (data.is_a<wayward::DateTime>()) {
        auto& datetime = *data.get<const DateTime&>();
        p.result = make_ast_literal<StringLiteral>(datetime.iso8601());
      } else {
        throw TypeError{wayward::format("Unsupported type in SQL literals: {0}", type->name())};
      }
    }

    bool can_modify() const {
      return false;
    }

    bool is_nil_at_current() const {
      return false;
    }
  };

  ast::Ptr<ast::SingleValue> DataAsLiteral::make_literal(AnyRef data, const IType* type) {
    Visitor visitor { *this };
    type->visit_data(data, visitor);
    return std::move(result);
  }
}

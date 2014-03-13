#include <persistence/relational_algebra.hpp>

namespace persistence {
  namespace relational_algebra {
    using w::make_cloning_ptr;

    Projection projection(std::string relation) {
      return Projection{relation};
    }

    Value column(std::string relation, std::string column) {
      return Value {
        make_cloning_ptr(
          new ast::ColumnReference{
            std::move(relation),
            std::move(column)
          }
        )};
    }

    Value literal(double n) {
      return Value {
        make_cloning_ptr(new ast::NumericLiteral{n})
      };
    }

    Condition operator&&(Condition&& lhs, Condition&& rhs) {
      return Condition{
        make_cloning_ptr(new ast::LogicalCondition{
          std::move(lhs.cond),
          std::move(rhs.cond),
          ast::LogicalCondition::AND
        })
      };
    }

    Condition operator||(Condition&& lhs, Condition&& rhs) {
      return Condition{
        make_cloning_ptr(new ast::LogicalCondition{
          std::move(lhs.cond),
          std::move(rhs.cond),
          ast::LogicalCondition::OR
        })
      };
    }

    Value::Value(const char* sql) {
      value = make_cloning_ptr(new ast::SQLFragmentValue{sql});
    }

    Value::Value(std::string sql) {
      value = make_cloning_ptr(new ast::SQLFragmentValue{std::move(sql)});
    }

    Condition Value::like(std::string literal) && {
      return Condition{make_cloning_ptr(new ast::BinaryCondition{
        std::move(value),
        make_cloning_ptr(new ast::StringLiteral{literal}),
        ast::BinaryCondition::Like
      })};
    }

    Condition Value::operator==(Value&& other) && {
      return Condition{make_cloning_ptr(new ast::BinaryCondition{
        std::move(value),
        std::move(other.value),
        ast::BinaryCondition::Eq
      })};
    }

    Condition Value::operator<(Value&& other) && {
      return Condition{make_cloning_ptr(new ast::BinaryCondition{
        std::move(value),
        std::move(other.value),
        ast::BinaryCondition::LessThan
      })};
    }

    Projection::Projection(std::string relation) {
      query = make_cloning_ptr(new ast::SelectQuery);
      query->relation = std::move(relation);
    }

    Projection Projection::where(Condition cond) const& {
      Projection p = *this;
      return std::move(p).where(std::move(cond));
    }

    Projection Projection::where(Condition cond) && {
      if (query->where) {
        auto new_cond = make_cloning_ptr(new ast::LogicalCondition{
          std::move(query->where),
          std::move(cond.cond),
          ast::LogicalCondition::AND
        });
        query->where = std::move(new_cond);
      } else {
        query->where = std::move(cond.cond);
      }
      return std::move(*this);
    }

    Projection Projection::order(Value ordering_value) const& {
      Projection p = *this;
      return std::move(p).order(std::move(ordering_value));
    }

    Projection Projection::order(Value ordering_value) && {
      query->order.clear();
      query->order.push_back(std::move(ordering_value.value));
      return std::move(*this);
    }

    Projection Projection::reverse_order(bool b) const& {
      Projection p = *this;
      return std::move(p).reverse_order(b);
    }

    Projection Projection::reverse_order(bool b) && {
      query->order_descending = b;
      return std::move(*this);
    }
  }
}

#include "persistence/adapters/postgresql/renderers.hpp"
#include <wayward/support/format.hpp>
#include <sstream>

namespace persistence {
    std::string PostgreSQLQueryRenderer::render(const SelectQuery& x) {
      PostgreSQLValueRenderer renderer{conn, symbolic_relation_resolver};
      std::stringstream ss;
      ss << "SELECT ";
      if (x.select.size()) {
        for (size_t i = 0; i < x.select.size(); ++i) {
          auto& alias = x.select[i];
          ss << alias.value->to_sql(renderer);
          if (alias.alias) {
            ss << wayward::format(" AS \"{0}\"", *alias.alias);
          }
          if (i+1 != x.select.size())
            ss << ", ";
        }
      } else {
        ss << "*";
      }
      ss << " FROM " << x.relation;
      if (x.relation_alias) {
        ss << " AS " << *x.relation_alias;
      }

      for (auto& join: x.joins) {
        switch (join->type) {
          case ast::Join::Cross:      ss << " CROSS JOIN "; break;
          case ast::Join::Inner:      ss << " INNER JOIN "; break;
          case ast::Join::LeftOuter:  ss << " LEFT OUTER JOIN "; break;
          case ast::Join::RightOuter: ss << " RIGHT OUTER JOIN "; break;
          case ast::Join::FullOuter:  ss << " FULL OUTER JOIN "; break;
        }
        ss << join->relation << " AS " << join->alias << " ON ";
        ss << join->on->to_sql(renderer);
      }

      if (x.where) {
        ss << " WHERE " << x.where->to_sql(renderer);
      }
      if (x.group.size()) {
        ss << " GROUP BY ";
        for (size_t i = 0; i < x.group.size(); ++i) {
          ss << x.group[i]->to_sql(renderer);
          if (i+1 != x.group.size())
            ss << ", ";
        }
      }
      if (x.order.size()) {
        ss << " ORDER BY ";
        for (size_t i = 0; i < x.order.size(); ++i) {
          ss << x.order[i].value->to_sql(renderer);
          if (x.order[i].ordering == ast::Ordering::Descending) {
            ss << " DESC";
          }
          if (i+1 != x.order.size())
            ss << ", ";
        }
      }
      if (x.limit) {
        ss << " LIMIT " << *x.limit;
        if (x.offset) {
          ss << " OFFSET " << *x.offset;
        }
      }
      return ss.str();
    }

    std::string PostgreSQLQueryRenderer::render(const UpdateQuery& x) {
      return "NIY";
    }

    std::string PostgreSQLQueryRenderer::render(const DeleteQuery& x) {
      return "NIY";
    }

    std::string PostgreSQLQueryRenderer::render(const InsertQuery& x) {
      std::stringstream ss;
      ss << "INSERT INTO " << x.relation << " (";
      for (auto it = x.columns.begin(); it != x.columns.end();) {
        ss << "\"" << *it << "\"";
        ++it;
        if (it != x.columns.end()) {
          ss << ", ";
        }
      }
      ss << ") VALUES (";
      PostgreSQLValueRenderer vr { conn, symbolic_relation_resolver };
      for (auto it = x.values.begin(); it != x.values.end();) {
        ss << (*it)->to_sql(vr);
        ++it;
        if (it != x.values.end()) {
          ss << ", ";
        }
      }
      ss << ")";

      if (x.returning_columns.size()) {
        ss << " RETURNING ";
        for (auto it = x.returning_columns.begin(); it != x.returning_columns.end();) {
          ss << "\"" << *it << "\"";
          ++it;
          if (it != x.returning_columns.end()) {
            ss << ", ";
          }
        }
      }

      return ss.str();
    }

    std::string PostgreSQLValueRenderer::render(const StarFrom& x) {
      return wayward::format("{0}.*", x.relation);
    }

    std::string PostgreSQLValueRenderer::render(const StringLiteral& x) {
      return wayward::format("'{0}'", conn.sanitize(x.literal));
    }

    std::string PostgreSQLValueRenderer::render(const NumericLiteral& x) {
      // TODO: Something cleverer once NumericLiteral becomes more aware of number types
      return wayward::format("{0}", x.literal);
    }

    std::string PostgreSQLValueRenderer::render(const BooleanLiteral& x) {
      return x.value ? "'t'" : "'f'";
    }

    std::string PostgreSQLValueRenderer::render(const ColumnReference& x) {
      return wayward::format("\"{0}\".\"{1}\"", x.relation, x.column);
    }

    std::string PostgreSQLValueRenderer::render(const ColumnReferenceWithSymbolicRelation& x) {
      return wayward::format("\"{0}\".\"{1}\"", symbolic_relation_resolver.relation_for_symbol(x.relation), x.column);
    }

    std::string PostgreSQLValueRenderer::render(const Aggregate& x) {
      std::stringstream ss;
      ss << x.function << '(';
      for (size_t i = 0; i < x.arguments.size(); ++i) {
        ss << x.arguments[i]->to_sql(*this);
        if (i+1 != x.arguments.size())
          ss << ", ";
      }
      ss << ')';
      return ss.str();
    }

    std::string PostgreSQLValueRenderer::render(const List& x) {
      std::stringstream ss;
      ss << '(';
      for (size_t i = 0; i < x.elements.size(); ++i) {
        ss << x.elements[i]->to_sql(*this);
        if (i+1 != x.elements.size())
          ss << ", ";
      }
      ss << ')';
      return ss.str();
    }

    std::string PostgreSQLValueRenderer::render(const CaseSimple& x) {
      return "NIY";
    }

    std::string PostgreSQLValueRenderer::render(const Case& x) {
      return "NIY";
    }

    std::string PostgreSQLValueRenderer::render(const NotCondition& x) {
      return wayward::format("NOT ({0})", x.subcondition->to_sql(*this));
    }

    std::string PostgreSQLValueRenderer::render(const UnaryCondition& x) {
      std::string op;
      switch (x.op) {
        case UnaryCondition::IsNull:       op = "IS NULL"; break;
        case UnaryCondition::IsNotNull:    op = "IS NOT NULL"; break;
        case UnaryCondition::IsTrue:       op = "IS TRUE"; break;
        case UnaryCondition::IsNotTrue:    op = "IS NOT TRUE"; break;
        case UnaryCondition::IsFalse:      op = "IS FALSE"; break;
        case UnaryCondition::IsNotFalse:   op = "IS NOT FALSE"; break;
        case UnaryCondition::IsUnknown:    op = "IS UNKNOWN"; break;
        case UnaryCondition::IsNotUnknown: op = "IS NOT UNKNOWN"; break;
      }
      return wayward::format("({0}) {1}", x.value->to_sql(*this));
    }

    std::string PostgreSQLValueRenderer::render(const BinaryCondition& x) {
      std::string op;
      switch (x.op) {
        case BinaryCondition::Eq:                op = "="; break;
        case BinaryCondition::NotEq:             op = "!="; break;
        case BinaryCondition::LessThan:          op = "<"; break;
        case BinaryCondition::GreaterThan:       op = ">"; break;
        case BinaryCondition::LessThanOrEq:      op = "<="; break;
        case BinaryCondition::GreaterThanOrEq:   op = ">="; break;
        case BinaryCondition::In:                op = "IN"; break;
        case BinaryCondition::NotIn:             op = "NOT IN"; break;
        case BinaryCondition::IsDistinctFrom:    op = "IS DISTINCT FROM"; break;
        case BinaryCondition::IsNotDistinctFrom: op = "IS NOT DISTINCT FROM"; break;
        case BinaryCondition::Like:              op = "LIKE"; break;
        case BinaryCondition::ILike:             op = "ILIKE"; break;
      }

      return wayward::format("{0} {1} {2}", x.lhs->to_sql(*this), op, x.rhs->to_sql(*this));
    }

    std::string PostgreSQLValueRenderer::render(const BetweenCondition& x) {
      return wayward::format("{0} IS BETWEEN {1} AND {2}", x.value->to_sql(*this), x.lower_bound->to_sql(*this), x.upper_bound->to_sql(*this));
    }

    std::string PostgreSQLValueRenderer::render(const LogicalCondition& x) {
      std::string op;
      switch (x.op) {
        case LogicalCondition::AND: op = "AND"; break;
        case LogicalCondition::OR:  op = "OR"; break;
      }
      return wayward::format("({0}) {1} ({2})", x.lhs->to_sql(*this), op, x.rhs->to_sql(*this));
    }

    std::string PostgreSQLValueRenderer::render(const SelectQuery& x) {
      // TODO: Actually, the semantics here may have to be slightly different, because
      // a sub-SELECT is allowed to do different things from a toplevel select.
      PostgreSQLQueryRenderer renderer{conn, symbolic_relation_resolver};
      return wayward::format("({0})", x.to_sql(renderer));
    }
}

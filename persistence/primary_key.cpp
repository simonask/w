#include "persistence/primary_key.hpp"
#include "persistence/property.hpp"

namespace persistence {
  const PrimaryKeyType* build_type(const TypeIdentifier<PrimaryKey>*) {
    static const PrimaryKeyType* p = new PrimaryKeyType;
    return p;
  }

  ast::Ptr<ast::SingleValue> PrimaryKeyType::make_literal(AnyConstRef data) const {
    if (!data.is_a<PrimaryKey>()) {
      throw TypeError("PrimaryKeyType::make_literal called with something that isn't a PrimaryKey.");
    }
    const PrimaryKey& pk = *data.get<const PrimaryKey&>();
    if (pk.is_persisted()) {
      return ast::Ptr<ast::SingleValue> { new ast::NumericLiteral { (double)pk.id } };
    }
    return ast::Ptr<ast::SingleValue> { new ast::SQLFragmentValue { "NULL" } };
  }
}

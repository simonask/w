#pragma once
#ifndef PERSISTENCE_ADAPTERS_POSTGRESQL_TYPE_MAPPER_HPP_INCLUDED
#define PERSISTENCE_ADAPTERS_POSTGRESQL_TYPE_MAPPER_HPP_INCLUDED

#include <persistence/sql_type_mapper.hpp>

namespace persistence {
  struct PostgreSQLTypeMapper : ISQLTypeMapper {
    wayward::CloningPtr<ast::SingleValue> literal_for_value(AnyConstRef value) final;
  };


}

#endif // PERSISTENCE_ADAPTERS_POSTGRESQL_TYPE_MAPPER_HPP_INCLUDED

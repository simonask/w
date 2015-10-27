#pragma once
#ifndef P_HPP_INCLUDED
#define P_HPP_INCLUDED

#include <persistence/association.hpp>
#include <persistence/record_type.hpp>
#include <persistence/connection.hpp>
#include <persistence/datetime.hpp>
#include <persistence/relational_algebra.hpp>
#include <persistence/projection.hpp>
#include <persistence/belongs_to.hpp>
#include <persistence/has_many.hpp>
#include <persistence/has_one.hpp>
#include <persistence/record.hpp>
#include <persistence/data_store.hpp>
#include <persistence/primary_key.hpp>
#include <persistence/record_type_builder.hpp>
#include <persistence/persistence_macro.hpp>
#include <persistence/record_as_structured_data.hpp>
#include <persistence/projection_as_structured_data.hpp>
#include <persistence/create.hpp>
#include <persistence/assign_attributes.hpp>
#include <persistence/validation_errors.hpp>

#if !defined(PERSISTENCE_NO_SHORTHAND_NAMESPACE)
namespace p = persistence;
#endif

#endif

#pragma once
#ifndef P_HPP_INCLUDED
#define P_HPP_INCLUDED

#include <persistence/type.hpp>
#include <persistence/belongs_to.hpp>
#include <persistence/has_many.hpp>
#include <persistence/has_one.hpp>
#include <persistence/record_type.hpp>
#include <persistence/record_type_builder.hpp>
#include <persistence/persistence_macro.hpp>

#if !defined(PERSISTENCE_NO_SHORTHAND_NAMESPACE)
namespace p = persistence;
#endif

#endif

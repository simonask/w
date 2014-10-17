from wayward_build import *
import os

wayward_support_sources = Split("""
  wayward/support/any.cpp
  wayward/support/benchmark.cpp
  wayward/support/format.cpp
  wayward/support/uri.cpp
  wayward/support/json.cpp
  wayward/support/datetime/datetime.cpp
  wayward/support/datetime/clock.cpp
  wayward/support/datetime/interval.cpp
  wayward/support/datetime/type.cpp
  wayward/support/logger.cpp
  wayward/support/error.cpp
  wayward/support/command_line_options.cpp
  wayward/support/fiber.cpp
  wayward/support/event_loop.cpp
  wayward/support/http.cpp
  wayward/support/teamwork.cpp
  wayward/support/plugin.cpp
  wayward/support/data_franca/spectator.cpp
  wayward/support/data_franca/mutator.cpp
  wayward/support/data_franca/object.cpp
  wayward/support/string.cpp
  wayward/support/types.cpp
  3rdparty/libevhtp/evhtp.c
  3rdparty/libevhtp/htparse/htparse.c
  3rdparty/libevhtp/evthr/evthr.c
  """)

wayward_support_headers = Split("""
  wayward/support/uri.hpp
  wayward/support/types.hpp
  wayward/support/type_info.hpp
  wayward/support/type.hpp
  wayward/support/thread_local.hpp
  wayward/support/teamwork.hpp
  wayward/support/string.hpp
  wayward/support/result.hpp
  wayward/support/plugin.hpp
  wayward/support/options.hpp
  wayward/support/monad.hpp
  wayward/support/meta.hpp
  wayward/support/maybe.hpp
  wayward/support/logger.hpp
  wayward/support/json.hpp
  wayward/support/intrusive_list.hpp
  wayward/support/http.hpp
  wayward/support/format.hpp
  wayward/support/fiber.hpp
  wayward/support/event_loop.hpp
  wayward/support/error.hpp
  wayward/support/either.hpp
  wayward/support/datetime.hpp
  wayward/support/data_visitor.hpp
  wayward/support/data_franca.hpp
  wayward/support/command_line_options.hpp
  wayward/support/cloning_ptr.hpp
  wayward/support/bitflags.hpp
  wayward/support/benchmark.hpp
  wayward/support/any.hpp
  wayward/support/data_franca/adapter.hpp
  wayward/support/data_franca/adapters.hpp
  wayward/support/data_franca/mutator.hpp
  wayward/support/data_franca/node.hpp
  wayward/support/data_franca/object.hpp
  wayward/support/data_franca/reader.hpp
  wayward/support/data_franca/spectator.hpp
  wayward/support/data_franca/types.hpp
  wayward/support/data_franca/writer.hpp
  wayward/support/datetime/clock.hpp
  wayward/support/datetime/datetime.hpp
  wayward/support/datetime/duration_units.hpp
  wayward/support/datetime/interval.hpp
  wayward/support/datetime/timezone.hpp
  wayward/support/datetime/type.hpp
  """)

wayward_sources = Split("""
  wayward/app.cpp
  wayward/render.cpp
  wayward/log.cpp
  wayward/template_engine.cpp
  wayward/respond_to.cpp
  wayward/content_type.cpp
  """)

wayward_headers = Split("""
  wayward/content_type.hpp
  wayward/respond_to.hpp
  wayward/routes.hpp
  wayward/session.hpp
  wayward/template_engine.hpp
  wayward/w.hpp
  w
  """)

w_util_sources = Split("""
  w_util/server.cpp
  w_util/recompiler.cpp
  w_util/main.cpp
  w_util/init.cpp
  """)

wayward_testing_sources = Split("""
  wayward/testing/time_machine.cpp
  """)

wayward_testing_headers = Split("""
  wayward/testing/time_machine.hpp
  """)

persistence_sources = Split("""
  persistence/connection_provider.cpp
  persistence/connection_retainer.cpp
  persistence/adapter.cpp
  persistence/data_store.cpp
  persistence/connection_pool.cpp
  persistence/p.cpp
  persistence/adapters/postgresql/connection.cpp
  persistence/adapters/postgresql/renderers.cpp
  persistence/primary_key.cpp
  persistence/relational_algebra.cpp
  persistence/insert.cpp
  persistence/property.cpp
  persistence/data_as_literal.cpp
  persistence/projection.cpp
  persistence/column.cpp
  persistence/assign_attributes.cpp
  """)

persistence_headers = Split("""
  persistence/adapters/postgresql/connection.hpp
  persistence/adapters/postgresql/renderers.hpp
  persistence/adapter.hpp
  persistence/assign_attributes.hpp
  persistence/association.hpp
  persistence/ast.hpp
  persistence/belongs_to.hpp
  persistence/column.hpp
  persistence/column_abilities.hpp
  persistence/column_traits.hpp
  persistence/connection.hpp
  persistence/connection_pool.hpp
  persistence/connection_provider.hpp
  persistence/connection_retainer.hpp
  persistence/context.hpp
  persistence/create.hpp
  persistence/data_as_literal.hpp
  persistence/data_store.hpp
  persistence/datetime.hpp
  persistence/destroy.hpp
  persistence/has_many.hpp
  persistence/has_one.hpp
  persistence/insert.hpp
  persistence/p.hpp
  persistence/persistence_macro.hpp
  persistence/primary_key.hpp
  persistence/projection.hpp
  persistence/projection_as_structured_data.hpp
  persistence/property.hpp
  persistence/record.hpp
  persistence/record_as_structured_data.hpp
  persistence/record_ptr.hpp
  persistence/record_type.hpp
  persistence/record_type_builder.hpp
  persistence/relational_algebra.hpp
  persistence/result_set.hpp
  persistence/validation_errors.hpp
  p
  """)

wayward_synth_sources = Split("""
  wayward/synth/synth.cpp
  """)

env = WaywardEnvironment(DefaultEnvironment())

wayward_support = WaywardLibrary(env, 'wayward_support', wayward_support_sources, wayward_support_headers)
env_with_ws = env.Clone()
env_with_ws.Append(LIBS = [wayward_support])
wayward         = WaywardLibrary(env_with_ws, 'wayward', wayward_sources, wayward_headers)
wayward_testing = WaywardLibrary(env_with_ws, 'wayward_testing', wayward_testing_sources, wayward_headers)

env_for_synth = env.Clone()
env_for_synth.Append(CPPPATH = Split('3rdparty/synth 3rdparty/synth/external/boost'))
wayward_synth   = WaywardPlugin(env_for_synth, 'wayward_synth', wayward_synth_sources)

if platform.system() == 'Linux':
  persistence_env = env
else:
  persistence_env = env.Clone()
  persistence_env.Append(LIBS = wayward_support)

persistence_env.Append(CCFLAGS = Split(libpq_cflags))
persistence_env.Append(LINKFLAGS = Split(libpq_libs))
persistence = WaywardLibrary(persistence_env, 'persistence', persistence_sources, persistence_headers)

WaywardAddDefaultLibraries([wayward, persistence, wayward_support])

w_dev = WaywardProgram(env_with_ws, 'w_dev', w_util_sources, rpaths = ['.'])

env.Alias('install', ['install-bin', 'install-lib', 'install-inc'])

Export('env', 'wayward', 'wayward_support', 'persistence', 'wayward_testing', 'WaywardProgram', 'WaywardInternalProgram', 'WaywardLibrary', 'WaywardEnvironment')

build_tests = ARGUMENTS.get('build_tests', 'yes')
if build_tests == 'yes':
  SConscript('tests/SConscript')



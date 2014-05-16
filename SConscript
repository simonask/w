import os
import platform

env = DefaultEnvironment()

wayward_support_sources = Split("""
  wayward/support/format.cpp
  wayward/support/uri.cpp
  wayward/support/node.cpp
  wayward/support/json.cpp
  wayward/support/datetime/datetime.cpp
  wayward/support/datetime/clock.cpp
  wayward/support/datetime/interval.cpp
  wayward/support/logger.cpp
  wayward/support/error.cpp
  wayward/support/command_line_options.cpp
  """)

wayward_sources = Split("""
  wayward/app.cpp
  wayward/render.cpp
  wayward/request.cpp
  wayward/log.cpp
  """)

w_util_sources = Split("""
  w_util/server.cpp
  w_util/recompiler.cpp
  w_util/main.cpp
  """)

wayward_testing_sources = Split("""
  wayward/testing/time_machine.cpp
  """)

persistence_sources = Split("""
  persistence/connection_provider.cpp
  persistence/connection_retainer.cpp
  persistence/adapter.cpp
  persistence/data_store.cpp
  persistence/connection_pool.cpp
  persistence/p.cpp
  persistence/postgresql.cpp
  persistence/postgresql_renderers.cpp
  persistence/primary_key.cpp
  persistence/relational_algebra.cpp
  persistence/types.cpp
  persistence/datetime.cpp
  """)

if platform.system() == 'Darwin':
  env.Replace(CXX = 'clang++')
  env.Replace(CC  = 'clang')
  env.Append(CFLAGS      = Split('-I/usr/local/include'))
  env.Append(LINKFLAGS   = Split('-rpath @loader_path/'))
  env.Append(SHLINKFLAGS = Split('-install_name @rpath/${TARGET.file}'))

env.Append(CCFLAGS = Split('-std=c++11'))
env.Append(CPPPATH = ['.'])

libevent_cflags = '`pkg-config --cflags libevent`'
libevent_libs   = '`pkg-config --libs libevent`'
libpq_cflags    = '`pkg-config --cflags libpq`'
libpq_libs      = '`pkg-config --libs libpq`'

env_with_libevent = env.Clone()
env_with_libevent.Append(CCFLAGS = libevent_cflags)
env_with_libevent.Append(LINKFLAGS = libevent_libs)

wayward_support = env_with_libevent.SharedLibrary("wayward_support", wayward_support_sources)
wayward         = env_with_libevent.SharedLibrary("wayward", wayward_sources, LIBS = wayward_support)
wayward_testing = env_with_libevent.SharedLibrary("wayward_testing", wayward_testing_sources, LIBS = wayward_support)
w_dev           = env_with_libevent.Program('w_dev', w_util_sources, LIBS = wayward_support)

persistence_env = env.Clone()
persistence_env.Append(CCFLAGS = libpq_cflags)
persistence_env.Append(LINKFLAGS = libpq_libs)
persistence     = persistence_env.SharedLibrary("persistence", persistence_sources, LIBS = wayward_support)

Export('env', 'wayward', 'wayward_support', 'persistence', 'wayward_testing')



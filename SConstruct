from wayward_build import *
import os

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
  wayward/support/fiber.cpp
  wayward/support/event_loop.cpp
  wayward/support/http.cpp
  wayward/support/teamwork.cpp
  3rdparty/libevhtp/evhtp.c
  3rdparty/libevhtp/htparse/htparse.c
  3rdparty/libevhtp/evthr/evthr.c
  """)

wayward_sources = Split("""
  wayward/app.cpp
  wayward/render.cpp
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

env = WaywardEnvironment(DefaultEnvironment())

wayward_support = WaywardLibrary(env, 'wayward_support', wayward_support_sources)
env_with_ws = env.Clone()
env_with_ws.Append(LIBS = [wayward_support])
wayward         = WaywardLibrary(env_with_ws, 'wayward', wayward_sources)
wayward_testing = WaywardLibrary(env_with_ws, 'wayward_testing', wayward_testing_sources)

if platform.system() == 'Linux':
  persistence_env = env
else:
  persistence_env = env.Clone()
  persistence_env.Append(LIBS = wayward_support)

persistence_env.Append(CCFLAGS = Split(libpq_cflags))
persistence_env.Append(LINKFLAGS = Split(libpq_libs))
persistence = WaywardLibrary(persistence_env, 'persistence', persistence_sources)

def WaywardProgram(environment, target_name, source, rpaths = []):
  linkflags = []
  env = environment.Clone()
  if platform.system() == 'Darwin':
    for path in rpaths:
      if not path.endswith('/'):
        path += '/'
      linkflags.extend(Split("-rpath @executable_path/" + path))
    env.Append(LIBS = [wayward, wayward_support])
  elif platform.system() == 'Linux':
    # Always include all libraries on Linux, because the GNU linker is being *so* *difficult*!
    # For instance, the system libraries (libevent and libpq) need to be at the end of the linker command, so we can't get
    # them as part of LINKFLAGS. This is because the GNU linker discards a library after having encountered it and
    # resolved any currently pending symbols.
    env.Append(LIBS = [wayward, persistence, wayward_support, 'event', 'event_pthreads', 'pq', 'unwind'])
  env.Append(LINKFLAGS = linkflags)
  return env.Program(target = target_name, source = source)


w_dev = WaywardProgram(env_with_ws, 'w_dev', w_util_sources, rpaths = ['.'])

Export('env', 'wayward', 'wayward_support', 'persistence', 'wayward_testing', 'WaywardProgram', 'WaywardLibrary', 'WaywardEnvironment')

build_tests = ARGUMENTS.get('build_tests', 'yes')
if build_tests == 'yes':
  SConscript('tests/SConscript')



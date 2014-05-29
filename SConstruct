import os
import platform
import copy

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

default_flags = {'cflags': [], 'ccflags': [], 'cxxflags': [], 'linkflags': []}
mode_flags = { 'release': copy.deepcopy(default_flags), 'development': copy.deepcopy(default_flags) }

if platform.system() == 'Darwin':
  env.Replace(CXX = 'clang++')
  env.Replace(CC  = 'clang')
  env.Append(CCFLAGS     = Split('-fcolor-diagnostics')) # SCons messes with the TTY so clang can't autodetect color capability.
  env.Append(CXXFLAGS    = Split('-std=c++11'))
  env.Append(CFLAGS      = Split('-I/usr/local/include'))
  env.Append(LINKFLAGS   = Split('-rpath @loader_path/'))
  env.Append(SHLINKFLAGS = Split('-install_name @rpath/${TARGET.file}'))
  mode_flags["release"]["ccflags"]     = Split("-O3 -g -flto -emit-llvm")
  mode_flags["release"]["linkflags"]   = Split("-O3 -g -flto")
  mode_flags["development"]["ccflags"] = Split("-O0 -g -DDEBUG=1")
elif platform.system() == 'Linux':
  env.Replace(CXX = 'clang++')
  env.Replace(CC  = 'clang')
  env.Append(CCFLAGS = '-fcolor-diagnostics')
  env.Append(CXXFLAGS = Split('-std=c++11 -stdlib=libc++'))
  env.Append(SHCCFLAGS = Split('-fPIC'))
  env.Append(SHLINKFLAGS = Split("-fvisibility=default -fPIC -soname '${TARGET.file}'"))
  env.Append(LINKFLAGS = Split("-pthread -stdlib=libc++"))
  mode_flags["release"]["ccflags"]     = Split("-O3 -g -flto")
  mode_flags["release"]["linkflags"]   = Split("-flto")
  mode_flags["development"]["ccflags"] = Split("-O0 -g -DDEBUG=1")
else:
  print "Error: Unsupported platform: " + platform.system()
  Exit(1)

mode = ARGUMENTS.get('mode', 'development')
if mode in mode_flags.keys():
  env.Append(CCFLAGS   = mode_flags[mode]["ccflags"])
  env.Append(CFLAGS    = mode_flags[mode]["cflags"])
  env.Append(CXXFLAGS  = mode_flags[mode]["cxxflags"])
  env.Append(LINKFLAGS = mode_flags[mode]["linkflags"])
else:
  print "Error: Invalid mode, expected 'development', 'release', got: " + mode
  Exit(1)

env.Append(CPPPATH = ['.'])

libevent_cflags = os.popen('pkg-config --cflags libevent libevent_pthreads').read().strip()
libevent_libs   = os.popen('pkg-config --libs libevent libevent_pthreads').read().strip()
libpq_cflags    = os.popen('pkg-config --cflags libpq').read().strip()
libpq_libs      = os.popen('pkg-config --libs libpq').read().strip()

env.Append(CCFLAGS = libevent_cflags)
env.Append(CCFLAGS = Split("-DEVHTP_DISABLE_REGEX -DEVHTP_DISABLE_SSL"))
env.Append(CPPPATH = Split("3rdparty/libevhtp 3rdparty/libevhtp/htparse 3rdparty/libevhtp/evthr"))
env.Append(LINKFLAGS = libevent_libs)

def WaywardLibrary(environment, target_name, source):
  if platform.system() == 'Darwin':
    return environment.SharedLibrary(target = target_name, source = source)
  elif platform.system() == 'Linux':
    return environment.StaticLibrary(target = target_name, source = source)

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

Export('env', 'wayward', 'wayward_support', 'persistence', 'wayward_testing', 'WaywardProgram')

build_tests = ARGUMENTS.get('build_tests', 'yes')
if build_tests == 'yes':
  SConscript('tests/SConscript')



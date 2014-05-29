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
  env.Append(CXXFLAGS = Split('-std=c++11'))
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
env.Append(CPPPATH = ['wayward/support'])

libevent_cflags = os.popen('pkg-config --cflags libevent libevent_pthreads').read().strip()
libevent_libs   = os.popen('pkg-config --libs libevent libevent_pthreads').read().strip()
libpq_cflags    = os.popen('pkg-config --cflags libpq').read().strip()
libpq_libs      = os.popen('pkg-config --libs libpq').read().strip()

env_with_libevent = env.Clone()
env_with_libevent.Append(CCFLAGS = libevent_cflags)
env_with_libevent.Append(CCFLAGS = Split("-DEVHTP_DISABLE_REGEX -DEVHTP_DISABLE_SSL"))
env_with_libevent.Append(CPPPATH = Split("3rdparty/libevhtp 3rdparty/libevhtp/htparse 3rdparty/libevhtp/evthr"))
env_with_libevent.Append(LINKFLAGS = libevent_libs)

libevhtp_sources = Split("""
  3rdparty/libevhtp/evhtp.c
  3rdparty/libevhtp/htparse/htparse.c
  3rdparty/libevhtp/evthr/evthr.c
  """)

libevhtp = env_with_libevent.StaticLibrary("libevhtp", libevhtp_sources)

wayward_support = env_with_libevent.SharedLibrary("wayward_support", wayward_support_sources, LIBS = libevhtp)
wayward         = env.SharedLibrary("wayward", wayward_sources, LIBS = wayward_support)
wayward_testing = env_with_libevent.SharedLibrary("wayward_testing", wayward_testing_sources, LIBS = wayward_support)
w_dev           = env_with_libevent.Program('w_dev', w_util_sources, LIBS = wayward_support)

persistence_env = env.Clone()
persistence_env.Append(CCFLAGS = libpq_cflags)
persistence_env.Append(LINKFLAGS = libpq_libs)
persistence     = persistence_env.SharedLibrary("persistence", persistence_sources, LIBS = wayward_support)

Export('env', 'wayward', 'wayward_support', 'persistence', 'wayward_testing')

build_tests = ARGUMENTS.get('build_tests', 'yes')
if build_tests == 'yes':
  SConscript('tests/SConscript')



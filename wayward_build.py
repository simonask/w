import os
import platform
import copy
from SCons.Script import *

libevent_cflags = os.popen('pkg-config --cflags libevent libevent_pthreads').read().strip()
libevent_libs   = os.popen('pkg-config --libs libevent libevent_pthreads').read().strip()
libpq_cflags    = os.popen('pkg-config --cflags libpq').read().strip()
libpq_libs      = os.popen('pkg-config --libs libpq').read().strip()

def WaywardLibrary(env, target, source):
  if platform.system() == 'Darwin':
    return env.SharedLibrary(target = target, source = source)
  elif platform.system() == 'Linux':
    return env.StaticLibrary(target = target, source = source)

def WaywardEnvironment(base):
  env = base.Clone()
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
  env.Append(CCFLAGS = libevent_cflags)
  env.Append(CCFLAGS = Split("-DEVHTP_DISABLE_REGEX -DEVHTP_DISABLE_SSL"))
  env.Append(CPPPATH = Split("3rdparty/libevhtp 3rdparty/libevhtp/htparse 3rdparty/libevhtp/evthr"))
  env.Append(LINKFLAGS = libevent_libs)
  return env

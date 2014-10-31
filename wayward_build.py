import os
import platform
import copy
from SCons.Script import *

opts = Variables()
opts.Add(PathVariable('PREFIX', 'Directory to install under', '/usr/local', PathVariable.PathIsDir))

lib_prefix = '$PREFIX/lib'
bin_prefix = '$PREFIX/bin'
inc_prefix = '$PREFIX/include'

libevent_cflags = os.popen('pkg-config --cflags libevent libevent_pthreads').read().strip()
libevent_libs   = os.popen('pkg-config --libs libevent libevent_pthreads').read().strip()
libpq_cflags    = os.popen('pkg-config --cflags libpq').read().strip()
libpq_libs      = os.popen('pkg-config --libs libpq').read().strip()

def WaywardLibrary(env, target, source, headers):
  opts.Update(env)
  if platform.system() == 'Darwin':
    lib = env.SharedLibrary(target = target, source = source)
  elif platform.system() == 'Linux':
    lib = env.StaticLibrary(target = target, source = source)

  for header in headers:
    subdir = os.path.dirname(header)
    path = inc_prefix + '/' + subdir
    env.Install(path, header)
    env.Alias('install-inc', path)

  env.Install(lib_prefix, lib)
  env.Alias('install-lib', lib_prefix)
  return lib

_wayward_default_libs = []

def WaywardAddDefaultLibraries(targets):
  _wayward_default_libs.extend(targets)

def WaywardInternalProgram(environment, target_name, source, rpaths = []):
  linkflags = []
  env = environment.Clone()
  if platform.system() == 'Darwin':
    for path in rpaths:
      if not path.endswith('/'):
        path += '/'
      linkflags.extend(Split("-rpath @executable_path/" + path))
    env.Append(LIBS = copy.copy(_wayward_default_libs))
  elif platform.system() == 'Linux':
    # Always include all libraries on Linux, because the GNU linker is being *so* *difficult*!
    # For instance, the system libraries (libevent and libpq) need to be at the end of the linker command, so we can't get
    # them as part of LINKFLAGS. This is because the GNU linker discards a library after having encountered it and
    # resolved any currently pending symbols.
    libs = copy.copy(_wayward_default_libs)
    libs.extend['event', 'event_pthreads', 'pq', 'unwind']
    env.Append(LIBS = libs)
  env.Append(LINKFLAGS = linkflags)
  return env.Program(target = target_name, source = source)

def WaywardProgram(environment, target_name, source, rpaths = []):
  opts.Update(environment)
  program = WaywardInternalProgram(environment, target_name, source, rpaths)
  environment.Install(bin_prefix, program)
  environment.Alias('install-bin', bin_prefix)
  return program

def WaywardPlugin(env, target, source):
  env = env.Clone()
  if platform.system() == 'Darwin':
    env.Replace(SHLINKFLAGS = '$LINKFLAGS -bundle -flat_namespace -undefined suppress')
    env.Replace(SHLIBSUFFIX = '.plugin')
    env.Replace(SHLIBPREFIX = '')
  elif platform.system() == 'Linux':
    env.Replace(SHLINKFLAGS = '$LINKFLAGS -shared')
    env.Append(SHCCFLAGS = "-fPIC")
    env.Replace(SHLIBSUFFIX = '.plugin')
    env.Replace(SHLIBPREFIX = '')
  return env.SharedLibrary(target, source)

def WaywardEnvironment(base):
  env = base.Clone()
  default_flags = {'cflags': [], 'ccflags': [], 'cxxflags': [], 'linkflags': []}
  mode_flags = { 'release': copy.deepcopy(default_flags), 'development': copy.deepcopy(default_flags) }

  if platform.system() == 'Darwin':
    env.Replace(CXX = 'clang++')
    env.Replace(CC  = 'clang')
    env.Append(CCFLAGS     = Split('-fcolor-diagnostics')) # SCons messes with the TTY so clang can't autodetect color capability.
    env.Append(CXXFLAGS    = Split('-std=c++1y'))
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
    env.Append(LINKFLAGS = Split("-pthread -stdlib=libc++ --export-dynamic"))
    env.Append(SHLINKFLAGS = Split("$LINKFLAGS -fvisibility=default -fPIC -soname '${TARGET.file}'"))
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
  env.Append(CPPPATH = Split("3rdparty/libevhtp 3rdparty/libevhtp/htparse 3rdparty/libevhtp/evthr"))
  env.Append(LINKFLAGS = libevent_libs)
  return env

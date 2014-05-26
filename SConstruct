env = DefaultEnvironment()

mode = ARGUMENTS.get('mode', 'development')

if mode == 'development':
  env.Append(CCFLAGS = Split('-O0 -g -DDEBUG=1'))
elif mode == 'production':
  env.Append(CCFLAGS = Split('-O3 -g -flto -emit-llvm'))
  env.Append(LINKFLAGS = Split('-O3 -flto'))
elif mode == 'staging':
  env.Append(CCFLAGS = Split('-O3 -g'))
else:
  print "Error: Expected 'development', 'production', or 'staging', got: " + mode
  Exit(1)

SConscript('SConscript')

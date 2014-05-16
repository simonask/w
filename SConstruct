env = DefaultEnvironment()

mode = ARGUMENTS.get('mode', 'development')

if mode == 'development':
  env.Append(CCFLAGS = Split('-O0 -g -DDEBUG=1'))
elif mode == 'production':
  env.Append(CCFLAGS = Split('-Os -g'))
elif mode == 'staging':
  env.Append(CCFLAGS = Split('-Os -g'))
else:
  print "Error: Expected 'development', 'production', or 'staging', got: " + mode
  Exit(1)

SConscript('SConscript', variant_dir = '.build.' + mode)

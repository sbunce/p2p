#Various functions to manipulate scons environments.

#If keys aren't defined this defines them.
def define_keys(env):
	if not env.has_key('LIBS'):
		env['LIBS'] = []
	if not env.has_key('LINKFLAGS'):
		env['LINKFLAGS'] = []
	if not env.has_key('LIBPATH'):
		env['LIBPATH'] = []
	if not env.has_key('CPPPATH'):
		env['CPPPATH'] = []
	if not env.has_key('CCFLAGS'):
		env['CCFLAGS'] = []

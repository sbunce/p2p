#if scons environment keys aren't defined then define them
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

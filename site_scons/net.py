#site_scons
import environment

#Add required include paths to environment.
def include(env):
	environment.define_keys(env)
	env['CPPPATH'].append('#libnet')

#Returns a path to static library.
def static_library():
	print 'libnet is header only'
	exit(1)

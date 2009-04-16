#site_scons
import environment

#Add required include paths to environment.
def include(env):
	environment.define_keys(env)
	env['CPPPATH'].append('#libsnet')

#Returns a path to static library.
def static_library():
	return '#libsnet/libsnet.a'

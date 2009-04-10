#site_scons
import environment

#Add required include paths to environment.
def include(env):
	environment.define_keys(env)
	env['CPPPATH'].append('#libsqlite3')

#Returns a path to static library.
def static_library():
	return '#libsqlite3/libsqlite3.a'

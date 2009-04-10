#site_scons
import environment

#Add required include paths to environment.
def include(env):
	environment.define_keys(env)
	env['CPPPATH'].append('#libtommath')

#Returns a path to static library.
def static_library():
	return '#libtommath/libtommath.a'

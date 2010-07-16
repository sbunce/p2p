#site_scons
import boost
import environment
import search
import parallel_build

#standard
import os
import platform
import string

def compile_flags(env):
	if platform.system() == 'Windows':
		env['CCFLAGS'].append([
			'/EHsc', #exception support
			'/w',    #disable warnings
			'/Ox'    #max optimization
		])
	else:
		env['CCFLAGS'].append('-O0')  #no optimization
		#env['CCFLAGS'].append('-O3') #all optimization

def system_library_path(env):
	if platform.system() == 'Windows':
		path = search.locate_dir_recurse(
			'/Program Files/Microsoft SDKs/Windows/', 'Lib')
		if not path:
			print 'error: could not locate windows libraries'
			exit(1)
		env['LIBPATH'].append(path)
	else:
		#make sure local libraries are favored
		env['LIBPATH'].append([
			'/usr/local/lib',
			'/usr/lib'
		])

def system_include_path(env):
	if platform.system() == 'Windows':
		path = search.locate_dir_recurse(
			'/Program Files/Microsoft SDKs/Windows/', 'Include')
		if not path:
			print 'error: could not locate windows headers'
			exit(1)
		env['CPPPATH'].append(path)
	else :
		#make sure local headers are favored
		env['CPPPATH'].append([
			'/usr/local/include',
			'/usr/include'
		])

def system_libraries(env):
	if platform.system() == 'Windows':
		env['LIBS'].append([
			'advapi32', #random number gen
			'ws2_32'    #winsock
		])   
	else:
		env['LIBS'].append('pthread')

def setup(env):
	environment.define_keys(env)

	#cache built objects for reuse if hash of source the same
	env.CacheDir('.cache')

	#don't always rehash stuff like #include <string>
	env.SetOption('implicit_cache', 1)

	#enable parallel building
	parallel_build.setup(env)

	#include path
	env['CPPPATH'].append('#include')
	system_include_path(env)
	boost.include_path(env)

	#library path
	env['LIBPATH'].append('#lib')
	system_library_path(env)
	boost.library_path(env)

	#libraries to link
	env['LIBS'].append([
		'p2p',
		'net',
		'tommath',
		'sqlite3',
		boost.library('boost_system'),
		boost.library('boost_filesystem'),
		boost.library('boost_regex'),
		boost.library('boost_thread')
	])
	system_libraries(env)

	#compile flags
	compile_flags(env)

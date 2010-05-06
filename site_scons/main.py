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
		env['CCFLAGS'].append('-O3')   #max optimization

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

	#cache dir for object reuse
	env.CacheDir('.cache')

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
		'boost_system',
		'boost_filesystem',
		'boost_regex',
		'boost_thread'
	])
	system_libraries(env)

	#compile flags
	compile_flags(env)

#WARNING: this can result in buggy builds (not threadsafe)
def setup_static(env):
	environment.define_keys(env)
	if platform.system() != 'Windows':
		env['LINKFLAGS'].append([
			'-static',
			'-static-libgcc',
			'`g++ -print-file-name=libstdc++.a`'
		])

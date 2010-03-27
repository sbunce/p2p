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
		env['CCFLAGS'].append('/EHsc') #exception support
		env['CCFLAGS'].append('/w')    #disable warnings
		env['CCFLAGS'].append('/Ox')   #max optimization
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
		env['LIBPATH'].append('/usr/local/lib')
		env['LIBPATH'].append('/usr/lib')

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
		env['CPPPATH'].append('/usr/local/include')
		env['CPPPATH'].append('/usr/include')

def system_libraries(env):
	if platform.system() == 'Windows':
		env['LIBS'].append('advapi32') #random number gen
		env['LIBS'].append('ws2_32')   #winsock
	else:
		env['LIBS'].append('pthread')

def setup(env):
	environment.define_keys(env)

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
	system_libraries(env)
	env['LIBS'].append('p2p')
	env['LIBS'].append('tommath')
	env['LIBS'].append('sqlite3')
	env['LIBS'].append(boost.library('boost_system'))
	env['LIBS'].append(boost.library('boost_filesystem'))
	env['LIBS'].append(boost.library('boost_regex'))
	env['LIBS'].append(boost.library('boost_thread'))

	#compile flags
	compile_flags(env)

#does static linking
def setup_static(env):
	environment.define_keys(env)
	if platform.system() != 'Windows':
		env['LINKFLAGS'].append('-static')
		env['LINKFLAGS'].append('-static-libgcc')
		env['LINKFLAGS'].append('`g++ -print-file-name=libstdc++.a`')

#site_scons
import boost
import environment
import search

#std
import os
import sys

#stuff every targets needs
def setup(env):
	environment.define_keys(env)

	#start minimum of 2 threads, more if > 2 CPUs
	num_cpu = int(os.environ.get('NUM_CPU', 2))
	env.SetOption('num_jobs', num_cpu)

	#custom, but independent libraries
	env['CPPPATH'].append([
		'#include',
		'#libnet',
		'#libp2p',
		'#libsqlite3',
		'#libtommath',
		boost.include()
	])

	#where to look for libraries
	#scons determines library build dependencies from this
	env['LIBPATH'].append([
		'#libp2p',
		'#libsqlite3',
		'#libtommath'
	])

	#libraries to link
	env['LIBS'] = [
		'p2p',
		'sqlite3',
		'tommath',
		boost.library('system'),
		boost.library('date_time'),
		boost.library('filesystem'),
		boost.library('thread')
	]

	#each supported platform should define to_upper(sys.platform)
	if sys.platform == 'linux2':
		env['LIBS'].append('pthread')
		env['CCFLAGS'].append('-O3')
		env['CCFLAGS'].append('-DLINUX2')
	if sys.platform == 'win32':
		env['LIBS'].append([
			'advapi32', #random number gen
			'ws2_32'    #winsock
		])   

		#find windows header location
		search_dir = '/Program Files/Microsoft SDKs/Windows/'
		found_dir = search.locate_dir_recurse(search_dir, 'Include')
		if not found_dir:
			print 'error: could not locate windows include directory'
			exit(1)
		env['CPPPATH'].append(found_dir)

		#find windows lib location
		search_dir = '/Program Files/Microsoft SDKs/Windows/'
		found_dir = search.locate_dir_recurse(search_dir, 'Lib')
		if not found_dir:
			print 'error: could not locate windows library directory'
			exit(1)
		env['LIBPATH'].append(found_dir)

		env['CCFLAGS'].append('/EHsc')   #exception support
		env['CCFLAGS'].append('/w')      #disable warnings
		env['CCFLAGS'].append('/Ox')     #max optimization
		env['CCFLAGS'].append('/DWIN32')

#can be run in addition to setup to do static linking
def setup_static(env):
	environment.define_keys(env)
	if sys.platform == 'linux2':
		env['LINKFLAGS'].append('-static')
		env['LINKFLAGS'].append('-static-libgcc')
		env['LINKFLAGS'].append('`g++ -print-file-name=libstdc++.a`')
	if sys.platform == 'win32':
		print 'TODO: windows static linking not supported'

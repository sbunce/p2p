#site_scons
import boost
import environment
import net
import p2p
import search
import sqlite3
import tommath

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
	env['CPPPATH'].append('#include')

	#all targets have access to these headers
	boost.include(env)
	net.include(env)
	p2p.include(env)
	sqlite3.include(env)
	tommath.include(env)

	#each supported platform should define to_upper(sys.platform)
	if sys.platform == 'linux2':
		env['CCFLAGS'].append('-O3')
		env['LIBS'].append('pthread')
		env['CCFLAGS'].append('-DLINUX2')
	if sys.platform == 'win32':
		#windows lib location
		search_dir = '/Program Files/Microsoft SDKs/Windows/'
		found_dir = search.locate_dir(search_dir, 'Lib')
		assert(found_dir)
		env['LIBPATH'].append(found_dir)

		#windows.h location
		search_dir = '/Program Files/Microsoft SDKs/Windows/'
		found_dir = search.locate_dir(search_dir, 'Include')
		assert(found_dir)
		env['CPPPATH'].append(found_dir)

		env['CCFLAGS'].append('/EHsc')   #exception support
		env['CCFLAGS'].append('/w')      #disable warnings
		env['CCFLAGS'].append('/Ox')     #max optimizations
		env['CCFLAGS'].append('/DWIN32')
		env['LIBS'].append('advapi32')   #random number gen
		env['LIBS'].append('ws2_32')     #winsock

#can be run in addition to setup to do static linking
def setup_static(env):
	environment.define_keys(env)
	if sys.platform == 'linux2':
		env['LINKFLAGS'].append('-static')
		env['LINKFLAGS'].append('-static-libgcc')
		env['LINKFLAGS'].append('`g++ -print-file-name=libstdc++.a`')
	if sys.platform == 'win32':
		print 'windows static linking not supported'
		exit(1)

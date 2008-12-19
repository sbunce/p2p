#Add system specific settings to a scons environment.

#site_scons
import environment
import search

#std
import os
import sys

#setup platform specific environment options
def env_setup(env):
	environment.define_keys(env)

	#start n build threads where n is the # of CPUs
	num_cpu = int(os.environ.get('NUM_CPU', 2))
	env.SetOption('num_jobs', num_cpu)

	if sys.platform == 'linux2':
		env['CCFLAGS'].append('-O3')   #max optimizations
		env['LIBS'].append('dl')       #needed for sqlite3
		env['LIBS'].append('pthread')  #needed for sqlite3
	if sys.platform == 'win32':
		env['LIBPATH'].append(__win32_lib_dir())
		env['CPPPATH'].append(__win32_include_dir())
		env['CCFLAGS'].append('/EHsc') #exception support
		env['CCFLAGS'].append('/w')    #disable warnings
		env['CCFLAGS'].append('/Ox')   #max optimizations
		env['LIBS'].append('ws2_32')   #winsock
		env['LIBS'].append('advapi32') #random number gen

def __win32_lib_dir():
	search_dir = '/Program Files/Microsoft SDKs/Windows/'
	found_dir = search.locate_dir(search_dir, 'Lib')
	if found_dir == '':
		print 'windows error: could not locate the windows API headers'
		exit(1)
	else:
		return found_dir

#returns a path to the windows API include path
def __win32_include_dir():
	search_dir = '/Program Files/Microsoft SDKs/Windows/'
	found_dir = search.locate_dir(search_dir, 'Include')
	if found_dir == '':
		print 'windows error: could not locate the windows API headers'
		exit(1)
	else:
		return found_dir

#site_scons
import environment
import search
import system

#std
import fnmatch
import os
import re

#Returns include path for boost. Terminates program if no boost headers found.
def include():
	#possible locations of headers
	if system.system() == 'linux':
		possible_locations = ['/usr/local/include', '/usr/include']
	if system.system() == 'windows':
		possible_locations = ['/Program Files/boost']

	pattern = 'boost[\_-]{1}[0-9]{1}_[0-9]{2}.*'
	for location in possible_locations:
		boost_dir = search.locate_dir(location, pattern)
		if boost_dir:
			return boost_dir

	print 'boost error: headers not found'
	exit(1)

#Returns name of library that is suitable to give to linker.
#Example: system -> boost_system-gcc43-mt-1_39
def library(lib_name):
	#possible location of library
	if system.system() == 'linux':
		possible_locations = ['/usr/local/lib', '/usr/lib']
	if system.system() == 'windows':
		possible_locations = ['/Program Files/boost/']

	pattern = 'libboost_' + lib_name + '.*'
	for location in possible_locations:
		lib_path = search.locate_file_recurse(location, pattern)
		if lib_path:
			(head, tail) = os.path.split(lib_path)
			if system.system() == 'windows':
				return tail
			else:
				lib_name = re.sub('lib', '', tail, 1)   #remove start of file name 'lib'
				lib_name = re.sub('\..*', '', lib_name) #remove end of file name .*
				return lib_name

	print 'boost error: could not locate static library with pattern ' + pattern
	exit(1)

def library_path():
	if system.system() == 'windows':
		possible_locations = ['/Program Files/boost']

		pattern = 'boost[\_-]{1}[0-9]{1}_[0-9]{2}.*'
		for location in possible_locations:
			boost_dir = search.locate_dir(location, pattern)
			if boost_dir and os.path.exists(os.path.join(boost_dir, 'lib')):
				return os.path.join(boost_dir, 'lib')
		print 'boost error: could not locate lib directory'

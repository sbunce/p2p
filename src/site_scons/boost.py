#site_scons
import environment
import search

#std
import fnmatch
import os
import sys

#Add required include paths to environment.
def include(env):
	environment.define_keys(env)

	#possible directories where the boost header directory resides
	if sys.platform == 'linux2':
		search_dir = '/usr/local/include'
		pattern = 'boost-[0-9]{1}_[0-9]{2}'
	if sys.platform == 'win32':
		search_dir = '/Program Files/boost'
		pattern = 'boost_[0-9]{1}_[0-9]{2}.*'

	found_dir = search.locate_dir(search_dir, pattern)
	if found_dir == '':
		print 'boost error: could not locate include directory'
		exit(1)
	else:
		env['CPPPATH'].append(found_dir)

#Returns a full path to the static lib given the library name.
#Example: thread, filesystem, system, etc
def static_library(library_name):
	pattern = 'libboost_'+library_name+'.*'

	#set up suffix to append to pattern
	if sys.platform == 'linux2':
		pattern += '\.a'
	if sys.platform == 'win32':
		pattern += '\.lib'

	#possible locations with boost library files
	if sys.platform == 'linux2':
		library_dir = '/usr/local/lib'
	if sys.platform == 'win32':
		#locate boost library directory boost_1_36_0
		library_dir = search.locate_dir('/Program Files/boost/', 'boost_[0-9]{1}_[0-9]{2}.*');
		library_dir += '/lib'

	#search directory for file name matching pattern
	library_path = search.locate_file(library_dir, pattern)
	if library_path == '':
		print 'boost error: could not locate static library with pattern ' + pattern
		exit(1)
	else:
		return library_path

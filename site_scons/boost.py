#site_scons
import environment
import search

#std
import fnmatch
import os
import sys

#Add required include paths to environment.
#/usr/local/include is prioritized over /usr/include
def include(env):
	environment.define_keys(env)

	if sys.platform == 'linux2':
		#try to locate boost in /usr/local/include
		search_dir = '/usr/local/include'
		pattern = 'boost-[0-9]{1}_[0-9]{2}'
		boost_dir = search.locate_dir(search_dir, pattern)
		if boost_dir:
			env['CPPPATH'].append(boost_dir)
			return

		#not in /usr/local/include, see if it exists in /usr/include
		search_dir = '/usr/include'
		pattern = 'boost'
		boost_dir = search.locate_dir(search_dir, pattern)
		if boost_dir:
			env['CPPPATH'].append(found_dir)
			return

		print 'boost error: could not locate include directory'
		exit(1)

	if sys.platform == 'win32':
		#try to locate boost in default http://www.boostpro.com/ install location
		search_dir = '/Program Files/boost'
		pattern = 'boost_[0-9]{1}_[0-9]{2}.*'
		boost_dir = search.locate_dir(search_dir, pattern)
		if boost_dir:
			env['CPPPATH'].append(found_dir)
		else:
			print 'boost error: could not locate include directory'
			exit(1)		

#Returns a full path to the static lib given the library name.
#Example: thread, filesystem, system, etc
def static_library(lib_name):
	if sys.platform == 'linux2':
		#try to locate boost in /usr/local/lib
		pattern = 'libboost_' + lib_name + '.*\.a'
		lib_dir = '/usr/local/lib'
		lib_path = search.locate_file(lib_dir, pattern)
		if lib_path:
			return lib_path

		#try to locate boost in /usr/lib
		lib_dir = '/usr/lib'
		lib_path = search.locate_file(lib_dir, pattern)
		if lib_path:
			return lib_path

		print 'boost error: could not locate static library with pattern ' + pattern
		exit(1)

	if sys.platform == 'win32':
		#try to locate boost in default http://www.boostpro.com/ install location
		pattern = 'libboost_' + lib_name + '.*\.lib'
		lib_dir = search.locate_dir('/Program Files/boost/', 'boost_[0-9]{1}_[0-9]{2}.*');
		lib_path = search.locate_file(lib_dir, pattern)
		if lib_path:
			return lib_path
		else:
			print 'boost error: could not locate static library with pattern ' + pattern
			exit(1)
			

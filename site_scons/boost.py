#site_scons
import environment
import search

#standard
import fnmatch
import os
import platform
import re

def include_path(env):
	if platform.system() == 'Windows':
		path = search.locate_dir('/Program Files/boost', 'boost.*')
		if path:
			env['CPPPATH'].append(path)
		else:
			print 'boost error: headers not found'
			exit(1)

def library_path(env):
	if platform.system() == 'Windows':
		path = search.locate_dir('/Program Files/boost', 'boost.*')
		if path and os.path.exists(os.path.join(path, 'lib')):
			env['LIBPATH'].append(os.path.join(path, 'lib'))
			return None
		else:
			print 'boost error: libraries not found'
			exit(1)

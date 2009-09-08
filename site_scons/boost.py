#site_scons
import environment
import search

#standard
import platform

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
		if path:
			env['LIBPATH'].append(path+'/lib')
		else:
			print 'boost error: libraries not found'
			exit(1)

def library(lib_name):
	if platform.system() == 'Windows':
		#windows does autolinking
		return ''
	else:
		return lib_name

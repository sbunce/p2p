#site_scons
import environment
import search

#std
import sys

def setup(env):
	environment.define_keys(env)
	if sys.platform == 'linux2':
		env.ParseConfig('pkg-config gtkmm-2.4 --cflags --libs')
	if sys.platform == 'win32':
		pkg_config = search.locate_file('/gtkmm/', 'pkg-config\.exe')
		if pkg_config == '':
			print 'gtkmm error: could not locate pkg-config'
			exit(1)

		env.ParseConfig(pkg_config + ' gtkmm-2.4 --cflags --libs')
		env['LIBPATH'].append('/gtkmm/lib')

#site_scons
import environment
import search
import system

#complete setup of env for gtkmm
def setup(env):
	environment.define_keys(env)

	if system.system() == 'linux':
		env.ParseConfig('pkg-config gtkmm-2.4 --cflags --libs')
	if system.system() == 'windows':
		pkg_config = search.locate_file_recurse('/gtkmm/', 'pkg-config\.exe')
		if pkg_config:
			env.ParseConfig(pkg_config + ' gtkmm-2.4 --cflags')
			env['LIBPATH'].append('/gtkmm/lib')

			#pkgconfig --lflags not working correctly, manually specify libs
			#DEBUG, try to get pkgconfig working
			env['LIBS'].append([
				'gtkmm-vc90-2_4',
				'gdkmm-vc90-2_4',
				'pangomm-vc90-1_4',
				'atkmm-vc90-1_6',
				'cairomm-vc90-1_0',
				'giomm-vc90-2_4',
				'glibmm-vc90-2_4',
				'sigc-vc90-2_0',
				'gtk-win32-2.0',
				'gdk_pixbuf-2.0',
				'pango-1.0',
				'pangocairo-1.0',
				'pangoft2-1.0',
				'pangowin32-1.0',
				'atk-1.0',
				'cairo',
				'gio-2.0',
				'gmodule-2.0',
				'gobject-2.0',
				'gthread-2.0',
				'glib-2.0',
				'libpng',
				'intl',
				'libtiff'
			])
		else:
			print 'gtkmm error: could not locate pkg-config'
			exit(1)

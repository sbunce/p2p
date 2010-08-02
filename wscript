import sys
from waflib.Tools import waf_unit_test

APPNAME='p2p'
VERSION='1.0.0'

top = '.'
out = 'build_dir'

def build(bld):
	bld.recurse('include')
	bld.recurse('lib')
	bld.recurse('front_end')
	bld(
		uselib = '',
		export_incdirs = 'include',
		name = 'local_include'
	)
	bld.add_post_fun(waf_unit_test.summary)

def locate_dir(search_dir, pattern):
	regex = re.compile(pattern)
	for root, dirs, files in os.walk(search_dir):
		for dir in dirs:
			if regex.match(dir):
				return os.path.join(root, dir)
		break #do not recurse to subdir

def configure(conf):
	conf.check_tool('compiler_cc')
	conf.check_tool('compiler_cxx')
	if sys.platform == 'win32':
		#platform
		conf.env.CXXFLAGS_platform = ['/EHsc'] 
		conf.env.LIB_platform = [
			'advapi32', #random number gen
			'ws2_32',   #winsock
		]
		#boost
		boost_dir = locate_dir('C:\Program Files\\boost', 'boost.*')
		conf.env.INCLUDES_boost = boost_dir
		conf.env.LIBPATH_boost = boost_dir + '\lib'
		#gtkmm ("pkg-config gtkmm-2.4 --libs" doesn't output correct names)
		conf.check_cfg(path='C:\gtkmm\\bin\pkg-config.exe', package='gtkmm-2.4', \
			args='--cflags', uselib_store='gtkmm', mandatory=True)
		conf.env.LIBPATH_gtkmm = ['C:\gtkmm\lib']
		conf.env.LIB_gtkmm = [
			'gtkmm-vc90-2_4', 'gdkmm-vc90-2_4', 'pangomm-vc90-1_4',
			'atkmm-vc90-1_6', 'cairomm-vc90-1_0', 'giomm-vc90-2_4',
			'glibmm-vc90-2_4', 'sigc-vc90-2_0', 'gtk-win32-2.0', 'gdk_pixbuf-2.0',
			'pango-1.0', 'pangocairo-1.0', 'pangoft2-1.0', 'pangowin32-1.0',
			'atk-1.0', 'cairo', 'gio-2.0', 'gmodule-2.0', 'gobject-2.0',
			'gthread-2.0', 'glib-2.0', 'libpng', 'intl', 'libtiff'
		]
	else:
		#boost
		conf.env.LIB_boost = [
			'pthread',
			'boost_system',
			'boost_filesystem',
			'boost_regex',
			'boost_thread'
		]
		#gtkmm
		conf.check_cfg(package='gtkmm-2.4', args='--libs --cflags', \
			uselib_store='gtkmm', mandatory=True)

def options(opt):
	opt.tool_options('compiler_cc')
	opt.tool_options('compiler_cxx')

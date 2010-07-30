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

def configure(conf):
	conf.check_tool('compiler_cc')
	conf.check_tool('compiler_cxx')
	conf.check_cfg(package='gtkmm-2.4', args='--libs --cflags', uselib_store='gtkmm', mandatory=True)

def options(opt):
	opt.tool_options('compiler_cc')
	opt.tool_options('compiler_cxx')

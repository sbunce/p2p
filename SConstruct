#site_scons
import main

#base environment(needed by all targets)
env_base = Environment()
main.setup(env_base)
Export('env_base')

print 'scons: starting',env_base.GetOption('num_jobs'),'builder'

SConscript([
	'gui_CLI/SConscript',
	'gui_gtkmm/SConscript',
	'http/SConscript',
	'include/SConscript',
	'lib_src/SConscript',
	'site_scons/SConscript'
])

Clean('bin','bin') #directory for built binaries
Clean('lib','lib') #directory for built libraries
Clean('.sconsign.dblite','.sconsign.dblite') #sqlite hash database

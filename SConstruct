#site_scons
import system

#base environment(needed by all targets)
env_base = Environment()
system.setup(env_base)
Export('env_base')

print 'scons: starting',env_base.GetOption('num_jobs'),'parallel jobs'

SConscript([
	'gui_CLI/SConscript',
	'gui_gtkmm/SConscript',
	'include/SConscript',
	'lib_src/SConscript',
	'site_scons/SConscript',
	'staging_area/SConscript'
])

Clean('bin','bin') #directory for built binaries
Clean('lib','lib') #directory for built libraries

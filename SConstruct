#site_scons
import system

#base environment(needed by all targets)
env_base = Environment()
system.setup(env_base)
Export('env_base')

print 'scons: starting',env_base.GetOption('num_jobs'),'parallel jobs'

SConscript([
	'gui_CLI/SConstruct',
	'gui_gtkmm/SConstruct',
	'include/SConstruct',
	'lib_src/SConstruct',
	'site_scons/SConstruct',

])

Clean('bin','bin') #directory for built binaries
Clean('lib','lib') #directory for built libraries

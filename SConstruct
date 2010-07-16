#site_scons
import main

#standard
import os

#base environment(needed by all targets)
env_base = Environment()
main.setup(env_base)
Export('env_base')

print 'scons: starting',env_base.GetOption('num_jobs'),'builders'

SConscript([
	'front_end/SConscript',
	'include/SConscript',
	'lib_src/SConscript',
	'site_scons/SConscript'
])

Clean('bin','bin')
Clean('lib','lib')
if env_base.GetOption('clean'):
	os.system('rm .sconsign.dblite')

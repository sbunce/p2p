#site_scons
import system

#base environment(needed by all targets)
env_base = Environment()
system.setup(env_base)
Export('env_base')

print 'building with',env_base.GetOption('num_jobs'),'threads'

SConscript([
	'gui/SConstruct',
	'include/SConstruct',
	'libnet/SConstruct',
	'libp2p/SConstruct',
	'libsqlite3/SConstruct',
	'libtommath/SConstruct',
	'site_scons/SConstruct',
])

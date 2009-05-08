#site_scons
import boost
import gtkmm
import sqlite3
import system
import tommath

#std
import sys

#base (needed by all targets)
env_base = Environment()
system.setup(env_base)

#static libc and libstdc++ linking
env_base_static = env_base.Clone()
system.setup_static(env_base_static)

#export base environment that all targets use
Export('env_base')
Export('env_base_static')

print 'building with',env_base.GetOption('num_jobs'),'threads'

#sub scons files
SConscript([
	'gui/SConstruct',
	'include/SConstruct',
	'libnet/SConstruct',
	'libp2p/SConstruct',
	'libsqlite3/SConstruct',
	'libtommath/SConstruct',
	'site_scons/SConstruct',
])

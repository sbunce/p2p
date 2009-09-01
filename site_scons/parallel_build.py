#standard
import platform
import sys
import StringIO

#source: TTimo
#http://scons.tigris.org/ds/viewMessage.do?dsForumId=1272&dsMessageId=421609
#This builder stops parallel build output from getting mixed together.
class builder_buffer:
	def buffered_spawn(self, sh, escape, cmd, args, env):
		stderr = StringIO.StringIO()
		stdout = StringIO.StringIO()
		command_string = ''
		for i in args:
			if(len(command_string)):
				command_string += ' '
			command_string += i
		try:
			retval = self.env['PSPAWN'](sh, escape, cmd, args, env, stdout, stderr)
		except OSError, x:
			if(x.errno != 10):
				raise x
			print 'OSError ignored on command: %s' % command_string
			retval = 0
		if(retval != 0):
			print command_string
			sys.stdout.write(stdout.getvalue())
			sys.stderr.write(stderr.getvalue())
		return retval

def CPU_count():
	if platform.system() == 'Linux':
		count = 0
		for line in open('/proc/cpuinfo', 'r'):
			if line.startswith('processor'):
				count += 1
		return count
	else:
		#assume only one CPU if we don't know
		return 1

#get a clean error output when running multiple jobs
def setup(env):
	BB = builder_buffer()
	BB.env = env
	env['SPAWN'] = BB.buffered_spawn
	env.SetOption('num_jobs', CPU_count())

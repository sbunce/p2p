#standard
import sys
import StringIO

#source: TTimo
#http://scons.tigris.org/ds/viewMessage.do?dsForumId=1272&dsMessageId=421609
class idBuffering:
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

#get a clean error output when running multiple jobs
def setup(env):
	buf = idBuffering()
	buf.env = env
	env['SPAWN'] = buf.buffered_spawn

#std
import os
import re
import sys
from SCons.Script.SConscript import SConsEnvironment

def unit_test(env, source):
	if sys.platform == 'win32':
		test = env.Program(source)
		env.AddPostAction(test, 'cd '+os.getcwd()+' && '+str(test[0]))
	else:
		#give posix systems a file extension that mercurial can ignore
		name = re.split('\.', source[0])
		name = name[0] + '.out'
		test = env.Program(name, source)
		env.AddPostAction(test, 'cd '+os.getcwd()+' && ./'+str(test[0]))

	return test

SConsEnvironment.unit_test = unit_test

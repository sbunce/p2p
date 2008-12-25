#std
import sys

from SCons.Script.SConscript import SConsEnvironment

def unit_test(env, source):
	test = env.Program(source)
	if sys.platform == 'linux2':
		env.AddPostAction(test, 'cd unit_tests && ./'+str(test[0]))
	if sys.platform == 'win32':
		env.AddPostAction(test, 'cd unit_tests && '+str(test[0]))
	return test

SConsEnvironment.unit_test = unit_test

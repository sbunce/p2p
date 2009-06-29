#standard
import glob
import os
import platform
import re
import sys
from SCons.Script.SConscript import SConsEnvironment

#Automated unit testing for a directory.
#This should be run in a unit_test directory SConscript
def run(env):
	for test in glob.iglob('*.cpp'):
		env.unit_test([test])

#unit test to hook in to scons
def __unit_test(env, source):
	if platform.system() == 'windows':
		test = env.Program(source)
		env.AddPostAction(test, 'cd '+os.getcwd()+' && '+str(test[0]))
	else:
		#give posix systems a file extension that mercurial can ignore
		name = re.split('\.', source[0])
		name = name[0] + '.out'
		test = env.Program(name, source)
		env.AddPostAction(test, 'cd '+os.getcwd()+' && ./'+str(test[0]))
	return test

SConsEnvironment.unit_test = __unit_test

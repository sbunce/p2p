#Various functions to do searches for directories and files.

#std
import os
import re

#Returns a fully qualified path to a directory within search_dir which
#corresponds to pattern. Recursively searches directories. Returns an empty
#string if no directory found.
def locate_dir(search_dir, pattern):
	regex = re.compile(pattern)
	return __locate_dir(search_dir, regex)

#private recursive function to locate directory
def __locate_dir(search_dir, regex):
	#make sure search_dir ends in '/'
	if search_dir[-1] != '/':
		search_dir += '/'

	#scan current directory
	for file in os.listdir(search_dir):
		if os.path.isdir(search_dir + file):
			if regex.match(file):
				#found directory that matches pattern
				return search_dir + file
			else:
				#recurse to new directory
				tmp = __locate_dir(search_dir + file, regex)
				if tmp != '':
					return tmp

	#no directory matching pattern found
	return '';

#Returns a fully qualified path to a file within search_dir that corresponds to
#pattern. Recursively searches directory. Returns and empty string if no file
#is found.
def locate_file(search_dir, pattern):
	regex = re.compile(pattern)
	return __locate_file(search_dir, regex)

#private recursive function to locate file
def __locate_file(search_dir, regex):
	#make sure search_dir ends in '/'
	if search_dir[-1] != '/':
		search_dir += '/'

	for file in os.listdir(search_dir):
		path = search_dir + file
		if os.path.isdir(path):
			tmp = locate_file(search_dir + file, regex)
			if tmp != '':
				return tmp
		else:
			if regex.match(file):
				return path

	#no file matching pattern found
	return ''

#Various functions to do searches for directories and files.

#std
import os
import re

#locate directory that matches pattern in specified search_dir
def locate_dir(search_dir, pattern):
	assert len(search_dir) > 0

	regex = re.compile(pattern)

	if search_dir[-1] != '/':
		search_dir += '/'

	for root, dirs, files in os.walk(search_dir):
		for dir in dirs:
			if regex.match(dir):
				return search_dir + dir
		break

	return False;

#recursivly locate directory that matches pattern starting at root_dir
def locate_dir_recurse(root_dir, pattern):
	assert len(root_dir) > 0

	regex = re.compile(pattern)

	if root_dir[-1] != '/':
		root_dir += '/'

	for root, dirs, files in os.walk(root_dir):
		for dir in dirs:
			if regex.match(dir):
				return root_dir + dir

	return False;

#locate file that matches pattern in specified search_dir
def locate_file(search_dir, pattern):
	assert len(search_dir) > 0

	regex = re.compile(pattern)

	if search_dir[-1] != '/':
		search_dir += '/'

	for root, dirs, files in os.walk(search_dir):
		for file in files:
			if regex.match(file):
				return search_dir + file
		break

	return False;

#recursivly locate file that matches pattern starting at root_dir
def locate_file_recurse(root_dir, pattern):
	assert len(root_dir) > 0

	regex = re.compile(pattern)

	if root_dir[-1] != '/':
		root_dir += '/'

	for root, dirs, files in os.walk(root_dir):
		for file in files:
			if regex.match(file):
				return root_dir + file

	return False;

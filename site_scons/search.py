#standard
import os
import re

#locate directory that matches pattern in specified search_dir
def locate_dir(search_dir, pattern):
	regex = re.compile(pattern)
	for root, dirs, files in os.walk(search_dir):
		for dir in dirs:
			if regex.match(dir):
				return os.path.join(root, dir)
		break #do not recurse to subdirectories
	return False;

#recursivly locate directory that matches pattern starting at root_dir
def locate_dir_recurse(root_dir, pattern):
	regex = re.compile(pattern)
	for root, dirs, files in os.walk(root_dir):
		for dir in dirs:
			if regex.match(dir):
				return os.path.join(root, dir)
	return False;

#locate file that matches pattern in specified search_dir
def locate_file(search_dir, pattern):
	regex = re.compile(pattern)
	for root, dirs, files in os.walk(search_dir):
		for file in files:
			if regex.match(file):
				return os.path.join(root, file)
		break #do not recurse to subdirectories
	return False;

#recursivly locate file that matches pattern starting at root_dir
def locate_file_recurse(root_dir, pattern):
	regex = re.compile(pattern)
	for root, dirs, files in os.walk(root_dir):
		for file in files:
			if regex.match(file):
				return os.path.join(root, file)
	return False;

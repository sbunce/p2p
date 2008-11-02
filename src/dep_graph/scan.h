#ifndef H_SCAN
#define H_SCAN

//std
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>

class scan
{
public:
	scan();
	~scan();

	/*
	run - starts scanning with root_file (file that contains main function)
	*/
	void run(std::string root_file);

private:

	//records all files that have been scanned
	std::set<std::string> scanned_files;

	/*
	eat_comment       - eats line comments and block comments
	read_preprocessor - reads #include "" preprocessor directives
	                    if a header is found it's pushed on to headers vector
	*/
	void eat_comment(std::fstream & fin);
	void read_preprocessor(std::fstream & fin, std::string & root_file, std::set<std::string> & scan);
};
#endif

#ifndef H_CLI_ARGS
#define H_CLI_ARGS

//std
#include <iostream>
#include <string>
#include <vector>

//custom
#include "global.h"

class CLI_args
{
public:
	//pass argc and argv strait from the main() function parameters
	CLI_args(const int & argc_in, char * argv_in[]);

	/*
	check_bool   - returns true if specified parameter was passed to program
	get_string   - sets str to string associated with param (ex: -path=/tra/la/la would set str to /tra/la/la)
	               returns false if string not found, else true
	help_message - prints a help message and exits the program if a specified parameter is found
	program_name - returns the program name
	*/
	bool check_bool(const std::string & param);
	bool get_string(const std::string & param, std::string & str);
	void help_message(const std::string & help_param, const std::string & help_message);
	const std::string & program_name();

private:
	//holds original arguments as passed in by ctor
	int argc;
	std::vector<std::string> argv;
};
#endif

//THREADSAFE
#ifndef H_CLI_ARGS
#define H_CLI_ARGS

//include
#include <boost/thread.hpp>

//standard
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

class CLI_args
{
public:
	//pass argc and argv strait from the main() function parameters
	CLI_args(const int argc_in, char * argv_in[]):
		argc(argc_in)
	{
		for(int x=0; x<argc; ++x){
			argv.push_back(std::string(argv_in[x]));
		}
	}

	//returns true if specified parameter was passed to program
	bool check_bool(const std::string & param)
	{
		std::vector<std::string>::iterator iter;
		iter = std::find(argv.begin(), argv.end(), param);
		return iter != argv.end();
	}

	/*
	Returns true and sets string if string parameter passed to program. Parameter
	must contain a zero before string. ex:
		--foo=bar
		Use "--foo" as param and str would be set to "bar".
	*/
	bool get_string(const std::string & param, std::string & str)
	{
		std::vector<std::string>::iterator iter_cur, iter_end;
		iter_cur = argv.begin();
		iter_end = argv.end();
		while(iter_cur != iter_end){
			if(iter_cur->compare(0, param.size(), param) == 0){
				str = iter_cur->substr(iter_cur->find_first_of('=')+1);
				return true;
			}
			++iter_cur;
		}
		return false;
	}

	/*
	Returns true and sets unsigned if parameter passed to program.
	*/
	bool get_uint(const std::string & param, unsigned & uint)
	{
		std::string temp_str;
		if(get_string(param, temp_str)){
			std::stringstream ss;
			ss << temp_str;
			ss >> uint;
			return true;
		}else{
			return false;
		}
	}

	//prints help message if specified help parameter exists
	void help_message(const std::string & help_param, const std::string & help_message)
	{
		if(check_bool(help_param)){
			std::cout << help_message;
			exit(0);
		}
	}

	//returns name of program (parameter 0)
	std::string program_name()
	{
		return argv[0];
	}
private:
	int argc;
	std::vector<std::string> argv;
};
#endif

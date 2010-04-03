//THREADSAFE
#ifndef H_CLI_ARGS
#define H_CLI_ARGS

//standard
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

class CLI_args
{
public:
	//pass in argc and argv from main()
	CLI_args(const int argc, char ** argv_in)
	{
		for(int x=0; x<argc; ++x){
			argv.push_back(std::string(argv_in[x]));
		}
	}

	/*
	Returns true if specified bool flag present.
	Example: "./a.out -b" requires param = "-b"
	*/
	bool flag(const std::string & param)
	{
		return std::find(argv.begin(), argv.end(), param) != argv.end();
	}

	/*
	Returns true and sets string if string parameter passed to program.
	Example: "./a.out --foo=bar" requires param = "--foo" to return "bar"
	*/
	bool string(const std::string & param, std::string & str)
	{
		for(std::vector<std::string>::iterator iter_cur = argv.begin(),
			iter_end = argv.end(); iter_cur != iter_end; ++iter_cur)
		{
			if(iter_cur->compare(0, param.size(), param) == 0){
				str = iter_cur->substr(iter_cur->find_first_of('=')+1);
				return true;
			}
		}
		return false;
	}

	/*
	Returns true and sets unsigned if parameter passed to program.
	Example: "./a.out --foo=123" requires param = "--foo" to return 123.
	*/
	bool uint(const std::string & param, unsigned & uint)
	{
		std::string temp;
		if(string(param, temp)){
			std::stringstream ss;
			ss << temp;
			ss >> uint;
			return true;
		}else{
			return false;
		}
	}

	/*
	If help parameter present this function prints help_message and terminates
	the program.
	*/
	void help_message(const std::string & help_param, const std::string & help_message)
	{
		if(flag(help_param)){
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
	std::vector<std::string> argv;
};
#endif

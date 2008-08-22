#include "CLI_args.h"

CLI_args::CLI_args(const int & argc_in, char * argv_in[])
: argc(argc_in)
{
	for(int x=0; x<argc; ++x){
		argv.push_back(std::string(argv_in[x]));
	}
}

bool CLI_args::check_bool(const std::string & param)
{
	std::vector<std::string>::iterator iter_cur, iter_end;
	iter_cur = argv.begin();
	iter_end = argv.end();
	while(iter_cur != iter_end){
		if(*iter_cur == param){
			return true;
		}
		++iter_cur;
	}
	return false;
}

bool CLI_args::get_string(const std::string & param, std::string & str)
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

void CLI_args::help_message(const std::string & help_param, const std::string & help_message)
{
	if(check_bool(help_param)){
		std::cout << help_message;
		exit(0);
	}
}

const std::string & CLI_args::program_name()
{
	return argv[0];
}

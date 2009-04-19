//custom
#include "http.hpp"

//include
#include <CLI_args.hpp>
#include <portable_sleep.hpp>

void run(const std::string & web_root, const unsigned max_upload_rate)
{
	http HTTP(web_root, max_upload_rate);
	while(true){
		HTTP.monitor();
		portable_sleep::ms(5000);
	}
}

int main(int argc, char ** argv)
{
	CLI_args CLI_Args(argc, argv);
	if(CLI_Args.check_bool("-test")){
		LOGGER << "woot";
	}

	std::string web_root;
	if(!CLI_Args.get_string("--web_root", web_root)){
		LOGGER << "web root not specified";
		exit(1);
	}

	unsigned max_upload_rate;
	if(!CLI_Args.get_uint("--max_upload_rate", max_upload_rate)){
		LOGGER << "max upload rate not specified";
		exit(1);
	}

	LOGGER << "web_root:        " << web_root;
	LOGGER << "max_upload_rate: " << max_upload_rate;

	run(web_root, max_upload_rate);
}

//include
#include <CLI_args.hpp>
#include <logger.hpp>

int fail(0);

int main()
{
	const char * argv[] = {"./a.out", "-b", "--x=123"};
	CLI_args CLI_Args(3, const_cast<char **>(argv));

	if(!CLI_Args.bool_flag("-b")){
		LOGGER; ++fail;
	}
	std::string str;
	if(!CLI_Args.string("--x", str)){
		LOGGER; ++fail;
	}
	if(str != "123"){
		LOGGER; ++fail;
	}

	unsigned uint;
	if(!CLI_Args.uint("--x", uint)){
		LOGGER; ++fail;
	}
	if(uint != 123){
		LOGGER; ++fail;
	}
	return fail;
}

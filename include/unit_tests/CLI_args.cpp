//include
#include <CLI_args.hpp>
#include <logger.hpp>

int fail(0);

int main()
{
	const char * argv[] = {"./a.out", "-b", "--x=123"};
	CLI_args CLI_Args(3, const_cast<char **>(argv));

	if(!CLI_Args.flag("-b")){
		LOGGER(logger::utest); ++fail;
	}
	std::string str;
	if(!CLI_Args.string("--x", str)){
		LOGGER(logger::utest); ++fail;
	}
	if(str != "123"){
		LOGGER(logger::utest); ++fail;
	}

	unsigned uint;
	if(!CLI_Args.uint("--x", uint)){
		LOGGER(logger::utest); ++fail;
	}
	if(uint != 123){
		LOGGER(logger::utest); ++fail;
	}
	return fail;
}

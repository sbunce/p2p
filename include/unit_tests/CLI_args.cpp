//include
#include <CLI_args.hpp>
#include <logger.hpp>

int fail(0);

int main()
{
	const char * argv[] = {"./a.out", "-b", "--x=123"};
	CLI_args CLI_Args(3, const_cast<char **>(argv));

	if(!CLI_Args.flag("-b")){
		LOG; ++fail;
	}
	std::string str;
	if(!CLI_Args.string("--x", str)){
		LOG; ++fail;
	}
	if(str != "123"){
		LOG; ++fail;
	}

	unsigned uint;
	if(!CLI_Args.uint("--x", uint)){
		LOG; ++fail;
	}
	if(uint != 123){
		LOG; ++fail;
	}
	return fail;
}

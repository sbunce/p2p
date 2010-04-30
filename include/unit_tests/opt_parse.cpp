//include
#include <opt_parse.hpp>
#include <logger.hpp>

int fail(0);

int main()
{
	const char * argv[] = {"./a.out", "-flag", "--flag", "--str0=123", "--str1",
		"123", "--str2123", "--num=123"};
	opt_parse OP(8, const_cast<char **>(argv));

	//composite flag
	if(!OP.flag('f')){
		LOG; ++fail;
	}
	if(!OP.flag('l')){
		LOG; ++fail;
	}
	if(!OP.flag('a')){
		LOG; ++fail;
	}
	if(!OP.flag('g')){
		LOG; ++fail;
	}

	//multi-char flag
	if(!OP.flag("flag")){
		LOG; ++fail;
	}

	//string
	boost::optional<std::string> str0 = OP.string("str0");
	if(!str0 || *str0 != "123"){
		LOG; ++fail;
	}
	boost::optional<std::string> str1 = OP.string("str1");
	if(!str1 || *str1 != "123"){
		LOG; ++fail;
	}
	boost::optional<std::string> str2 = OP.string("str2");
	if(!str2 || *str2 != "123"){
		LOG; ++fail;
	}

	//lexical_cast
	boost::optional<boost::uint32_t> num = OP.lexical_cast<boost::uint32_t>("num");
	if(!num || *num != 123){
		LOG; ++fail;
	}

	//make sure all options parsed
	if(OP.unparsed()){
		LOG; ++fail;
	}

	return fail;
}

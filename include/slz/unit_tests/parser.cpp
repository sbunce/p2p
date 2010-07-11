//include
#include <slz/slz.hpp>

int fail(0);

class test : public slz::message
{
public:
	test()
	{
		add_field(ASCII);
		add_field(string);
		add_field(uint);
		add_field(sint);
	}
	slz::ASCII<0> ASCII;
	slz::string<1> string;
	slz::uint<2> uint;
	slz::sint<3> sint;
};

int main()
{
	std::string test_string =
		" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNO"
		"PQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
	unsigned test_uint = 123;
	int test_sint = -123;

	boost::shared_ptr<test> Test(new test());
	Test->ASCII = test_string;
	Test->string = test_string;
	Test->uint = test_uint;
	Test->sint = test_sint;

	slz::parser Parser;
	Parser.add_message(Test);
	std::string buf;
	buf += Test->serialize();
	buf += Test->serialize();
	unsigned test_parsed = 0;
	while(boost::shared_ptr<slz::message> M = Parser.parse(buf)){
		if(typeid(*M) == typeid(test)){
			test * Test = (test *)M.get();
			if(!Test->ASCII || *Test->ASCII != test_string){
				LOG; ++fail;
			}
			if(!Test->string || *Test->string != test_string){
				LOG; ++fail;
			}
			if(!Test->uint || *Test->uint != test_uint){
				LOG; ++fail;
			}
			if(!Test->sint || *Test->sint != test_sint){
				LOG; ++fail;
			}
			++test_parsed;
		}
	}
	if(test_parsed != 2){
		LOG; ++fail;
	}
	if(!Parser.good()){
		LOG; ++fail;
	}

	return fail;
}

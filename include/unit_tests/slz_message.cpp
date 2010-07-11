//include
#include <slz.hpp>

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
	slz::vector<slz::ASCII<4> > ASCII_vec;
};

int main()
{
	std::string test_str =
		" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNO"
		"PQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
	
	test Test, par_Test;
	Test.ASCII = test_str;
	Test.string = test_str;
	Test.uint = 123;
	Test.sint = -123;
	Test.ASCII_vec->push_back(test_str);
	Test.ASCII_vec->push_back(test_str);

	if(!par_Test.parse(Test.serialize())){
		LOG; ++fail;
	}
	if(Test != par_Test){
		LOG; ++fail;
	}

	return fail;
}

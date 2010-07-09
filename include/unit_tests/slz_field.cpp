//include
#include <slz.hpp>

int fail(0);

int main()
{
	std::string buf;
	std::string test_string = 
		" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNO"
		"PQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
	unsigned test_uint = 123;
	int test_sint = -123;

	//ASCII
	slz::ASCII<0> ASCII;
	ASCII = test_string;
	buf = ASCII.serialize();
	if(!ASCII.parse(buf)){
		LOG; ++fail;
	}
	if(!ASCII || *ASCII != test_string){
		LOG; ++fail;
	}

	//uint
	slz::uint<0> uint;
	uint = test_uint;
	buf = uint.serialize();
	if(!uint.parse(buf)){
		LOG; ++fail;
	}
	if(!uint || *uint != test_uint){
		LOG; ++fail;
	}

	//sint
	slz::sint<0> sint;
	sint = test_sint;
	buf = sint.serialize();
	if(!sint.parse(buf)){
		LOG; ++fail;
	}
	if(!sint || *sint != test_sint){
		LOG; ++fail;
	}

	//string
	slz::string<0> string;
	string = test_string;
	buf = string.serialize();
	if(!string.parse(buf)){
		LOG; ++fail;
	}
	if(!string || *string != test_string){
		LOG; ++fail;
	}

	return fail;
}

//include
#include <slz/slz.hpp>

int fail(0);

int main()
{
	//ASCII string for test
	std::string test_str = 
		" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNO"
		"PQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";

	//ASCII
	slz::ASCII<0> ASCII = test_str, par_ASCII;
	if(!par_ASCII.parse(ASCII.serialize())){
		LOG; ++fail;
	}
	if(!ASCII || !par_ASCII || *ASCII != *par_ASCII){
		LOG; ++fail;
	}

	//bool
	slz::boolean<0> boolean = true, par_boolean;
	if(!par_boolean.parse(boolean.serialize())){
		LOG; ++fail;
	}
	if(!boolean || !par_boolean || *boolean != *par_boolean){
		LOG; ++fail;
	}
	boolean = false;
	if(!par_boolean.parse(boolean.serialize())){
		LOG; ++fail;
	}
	if(!boolean || !par_boolean || *boolean != *par_boolean){
		LOG; ++fail;
	}

	//uint
	slz::uint<0> uint = 123, par_uint;
	if(!par_uint.parse(uint.serialize())){
		LOG; ++fail;
	}
	if(!uint || !par_uint || *uint != *par_uint){
		LOG; ++fail;
	}

	//sint
	slz::sint<0> sint = -123, par_sint;
	if(!par_sint.parse(sint.serialize())){
		LOG; ++fail;
	}
	if(!sint || !par_sint || *sint != *par_sint){
		LOG; ++fail;
	}

	//string
	slz::string<0> string = test_str, par_string;
	if(!par_string.parse(string.serialize())){
		LOG; ++fail;
	}
	if(!string || !par_string || *string != *par_string){
		LOG; ++fail;
	}

	//vector of vints
	slz::vector<slz::uint<0> > uint_vec, par_uint_vec;
	uint_vec->push_back(0);
	uint_vec->push_back(1);
	uint_vec->push_back(2);
	if(!par_uint_vec.parse(uint_vec.serialize())){
		LOG; ++fail;
	}
	if(*uint_vec != *par_uint_vec){
		LOG; ++fail;
	}

	//vector of variable length fields
	slz::vector<slz::string<0> > string_vec, par_string_vec;
	string_vec->push_back("ABCD");
	string_vec->push_back("123");
	string_vec->push_back("VWXYZ");
	if(!par_string_vec.parse(string_vec.serialize())){
		LOG; ++fail;
	}
	if(*string_vec != *par_string_vec){
		LOG; ++fail;
	}

	//vector composition
	slz::vector<slz::vector<slz::uint<0> > > uint_vv, par_uint_vv;
	slz::vector<slz::uint<0> > uint_v;
	uint_v->push_back(0);
	uint_v->push_back(1);
	uint_v->push_back(2);
	uint_vv->push_back(uint_v);
	uint_vv->push_back(uint_v);
	uint_vv->push_back(uint_v);
	if(!par_uint_vv.parse(uint_vv.serialize())){
		LOG; ++fail;
	}
	if(*uint_vv != *par_uint_vv){
		LOG; ++fail;
	}

	return fail;
}

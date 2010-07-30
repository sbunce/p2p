#include <cpproto/cpproto.hpp>
#include <logger.hpp>

int main()
{
	int fail_cnt = 0;
	cpproto::list<cpproto::uint<0> > field, parsed_field;
	field->push_back(0);
	field->push_back(1);
	field->push_back(2);
	if(!parsed_field.parse(field.serialize())){
		LOG; ++fail_cnt;
	}
	if(!field || !parsed_field || *field != *parsed_field){
		LOG; ++fail_cnt;
	}
	return fail_cnt;
}

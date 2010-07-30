#include <cpproto/cpproto.hpp>
#include <logger.hpp>

int main()
{
	int fail_cnt = 0;
	cpproto::boolean<0> field = true, parsed_field;
	if(!parsed_field.parse(field.serialize())){
		LOG; ++fail_cnt;
	}
	if(!field || !parsed_field || *field != *parsed_field){
		LOG; ++fail_cnt;
	}
	field = false;
	if(!parsed_field.parse(field.serialize())){
		LOG; ++fail_cnt;
	}
	if(!field || !parsed_field || *field != *parsed_field){
		LOG; ++fail_cnt;
	}
	return fail_cnt;
}

#include <cpproto/cpproto.hpp>
#include <logger.hpp>

int main()
{
	int fail_cnt = 0;
	cpproto::uint<0> unknown_field(1234);
	cpproto::unknown field, parsed_field;
	field.parse(unknown_field.serialize());
	if(!parsed_field.parse(field.serialize())){
		LOG; ++fail_cnt;
	}
	if(!field || !parsed_field || field != parsed_field){
		LOG; ++fail_cnt;
	}
	if(parsed_field.ID() != 0){
		LOG; ++fail_cnt;
	}
	return fail_cnt;
}

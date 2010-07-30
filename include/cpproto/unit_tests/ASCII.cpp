#include <cpproto/cpproto.hpp>
#include <logger.hpp>

int main()
{
	int fail_cnt = 0;
	const std::string test_str(
		" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNO"
		"PQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"
	);
	cpproto::ASCII<0> field = test_str, parsed_field;
	if(!parsed_field.parse(field.serialize())){
		LOG; ++fail_cnt;
	}
	if(!field || !parsed_field || *field != *parsed_field){
		LOG; ++fail_cnt;
	}
	return fail_cnt;
}

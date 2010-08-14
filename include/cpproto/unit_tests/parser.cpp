#include <cpproto/cpproto.hpp>
#include <logger.hpp>

CPPROTO_MESSAGE_BEGIN(message, 0)
	CPPROTO_FIELD(cpproto::ASCII<0>, ASCII)
	CPPROTO_FIELD(cpproto::boolean<1>, boolean)
	CPPROTO_FIELD(cpproto::string<2>, string)
	CPPROTO_FIELD(cpproto::sint<3>, sint)
	CPPROTO_FIELD(cpproto::uint<4>, uint)
CPPROTO_MESSAGE_END

const std::string test_str(
	" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNO"
	"PQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"
);

void message_set_test_vals(message & M)
{
	M.ASCII = test_str;
	M.boolean = true;
	M.string = test_str;
	M.sint = -123;
	M.uint = 123;
}

int main()
{
	int fail_cnt = 0;
	cpproto::parser Parser;
	Parser.add_field<message>();
	message field;
	message_set_test_vals(field);
	std::string buf;
	buf += field.serialize();
	buf += field.serialize();
	unsigned test_parsed = 0;
	while(boost::shared_ptr<cpproto::field> M = Parser.parse(buf)){
		if(typeid(*M) == typeid(message)){
			message * parsed_field = (message *)M.get();
			if(field != *parsed_field){
				LOG; ++fail_cnt;
			}
			++test_parsed;
		}
	}
	if(test_parsed != 2){
		LOG; ++fail_cnt;
	}
	if(Parser.bad_stream()){
		LOG; ++fail_cnt;
	}
	return fail_cnt;
}

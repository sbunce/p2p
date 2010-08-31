#include <cpproto/cpproto.hpp>
#include <logger.hpp>

int fail_cnt(0);
int call_back_cnt(0);

CPPROTO_MESSAGE_BEGIN(message, 0)
	CPPROTO_FIELD(cpproto::ASCII<0>, ASCII)
	CPPROTO_FIELD(cpproto::boolean<1>, boolean)
	CPPROTO_FIELD(cpproto::string<2>, string)
	CPPROTO_FIELD(cpproto::sint<3>, sint)
	CPPROTO_FIELD(cpproto::uint<4>, uint)
CPPROTO_MESSAGE_END
message M1, M2;

void call_back(message & M)
{
	if(!(M == M1 || M == M2)){
		LOG; ++fail_cnt;
	}
	++call_back_cnt;
}

int main()
{
	cpproto::parser Parser;
	Parser.reg_handler<message>(&call_back);

	M1.ASCII = "ABC";
	M1.boolean = true;
	M1.string = "123";
	M1.sint = -123;
	M1.uint = 123;

	M2.ASCII = "CBA";
	M2.boolean = false;
	M2.string = "321";
	M2.sint = -321;
	M2.uint = 321;

	std::string buf;
	buf = M1.serialize() + M2.serialize();
	Parser.parse(buf);

	if(Parser.bad()){
		LOG; ++fail_cnt;
	}

	if(call_back_cnt != 2){
		LOG; ++fail_cnt;
	}

	return fail_cnt;
}

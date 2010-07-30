#include <cpproto/cpproto.hpp>
#include <logger.hpp>

//old message version
CPPROTO_MESSAGE_BEGIN(m_old, 0)
	CPPROTO_FIELD(cpproto::string<0>, string)
CPPROTO_MESSAGE_END

//new message version that has additional field
CPPROTO_MESSAGE_BEGIN(m_new, 0)
	CPPROTO_FIELD(cpproto::string<0>, string)
	CPPROTO_FIELD(cpproto::uint<1>, uint)
CPPROTO_MESSAGE_END

int main()
{
	int fail_cnt = 0;
	m_old M_Old;
	m_new M_New;
	M_New.string = "ABC";
	M_New.uint = 123;
	if(!M_Old.parse(M_New.serialize())){
		LOG; ++fail_cnt;
	}
	//M_Old holds unknown field
	if(M_Old != M_New){
		LOG; ++fail_cnt;
	}
	if(M_Old != M_New){
		LOG; ++fail_cnt;
	}
	return fail_cnt;
}

#include <cpproto/cpproto.hpp>
#include <logger.hpp>

CPPROTO_MESSAGE_BEGIN(nested_message, 0)
	CPPROTO_FIELD(cpproto::ASCII<0>, ASCII)
	CPPROTO_FIELD(cpproto::boolean<1>, boolean)
	CPPROTO_FIELD(cpproto::string<2>, string)
	CPPROTO_FIELD(cpproto::sint<3>, sint)
	CPPROTO_FIELD(cpproto::uint<4>, uint)
	CPPROTO_FIELD(cpproto::list<cpproto::ASCII<5> >, ASCII_list)
CPPROTO_MESSAGE_END

CPPROTO_MESSAGE_BEGIN(message, 1)
	CPPROTO_FIELD(cpproto::ASCII<1>, ASCII)
	CPPROTO_FIELD(cpproto::boolean<2>, boolean)
	CPPROTO_FIELD(cpproto::string<3>, string)
	CPPROTO_FIELD(cpproto::sint<4>, sint)
	CPPROTO_FIELD(cpproto::uint<5>, uint)
	CPPROTO_FIELD(cpproto::list<cpproto::ASCII<6> >, ASCII_list)
	CPPROTO_FIELD(cpproto::list<nested_message>, Nested_Message_list)
CPPROTO_MESSAGE_END

const std::string test_str(
	" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNO"
	"PQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"
);

void nested_message_set_test_vals(nested_message & NM)
{
	NM.ASCII = test_str;
	NM.boolean = false;
	NM.string = test_str;
	NM.uint = 123;
	NM.sint = -123;
	NM.ASCII_list->push_back(test_str);
	NM.ASCII_list->push_back(test_str);
}

void message_set_test_vals(message & M)
{
	M.ASCII = test_str;
	M.boolean = true;
	M.string = test_str;
	M.uint = 123;
	M.sint = -123;
	M.ASCII_list->push_back(test_str);
	M.ASCII_list->push_back(test_str);
	nested_message tmp;
	nested_message_set_test_vals(tmp);
	M.Nested_Message_list->push_back(tmp);
	tmp.uint = 321;
	M.Nested_Message_list->push_back(tmp);
}

int main()
{
	int fail_cnt = 0;
	message field, parsed_field;
	message_set_test_vals(field);
	if(!parsed_field.parse(field.serialize())){
		LOG; ++fail_cnt;
	}
	if(field != parsed_field){
		LOG; ++fail_cnt;
	}
	return fail_cnt;
}

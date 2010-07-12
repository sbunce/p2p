//include
#include <slz/slz.hpp>

int fail(0);

class nested_message : public slz::message<6>
{
public:
	nested_message()
	{
		add_field(ASCII);
		add_field(string);
		add_field(uint);
		add_field(sint);
	}
	slz::ASCII<7> ASCII;
	slz::string<2> string;
	slz::uint<3> uint;
	slz::sint<4> sint;
	slz::vector<slz::ASCII<5> > ASCII_vec;
};

class message : public slz::message<0>
{
public:
	message()
	{
		add_field(ASCII);
		add_field(string);
		add_field(uint);
		add_field(sint);
		add_field(Nested_Message);
		add_field(Nested_Message_vec);
	}
	slz::ASCII<1> ASCII;
	slz::string<2> string;
	slz::uint<3> uint;
	slz::sint<4> sint;
	slz::vector<slz::ASCII<5> > ASCII_vec;
	nested_message Nested_Message;
	slz::vector<nested_message> Nested_Message_vec;
};

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

	//message	
	boost::shared_ptr<message> Message(new message()), par_Message(new message());
	//message data members
	Message->ASCII = test_str;
	Message->string = test_str;
	Message->uint = 123;
	Message->sint = -123;
	Message->ASCII_vec->push_back(test_str);
	Message->ASCII_vec->push_back(test_str);
	//nested message data members
	Message->Nested_Message.ASCII = test_str;
	Message->Nested_Message.string = test_str;
	Message->Nested_Message.uint = 123;
	Message->Nested_Message.sint = -123;
	Message->Nested_Message.ASCII_vec->push_back(test_str);
	Message->Nested_Message.ASCII_vec->push_back(test_str);
	//vector of messages
	Message->Nested_Message_vec->push_back(Message->Nested_Message);
	Message->Nested_Message_vec->push_back(Message->Nested_Message);
	if(!par_Message->parse(Message->serialize())){
		LOG; ++fail;
	}
	if(*Message != *par_Message){
		LOG; ++fail;
	}

	//parser
	slz::parser Parser;
	Parser.add_field(Message);
	std::string buf;
	buf += Message->serialize();
	buf += Message->serialize();
	unsigned test_parsed = 0;
	while(boost::shared_ptr<slz::field> M = Parser.parse(buf)){
		if(typeid(*M) == typeid(message)){
			message * par_Message = (message *)M.get();
			if(*par_Message != *Message){
				LOG; ++fail;
			}
			++test_parsed;
		}
	}
	if(test_parsed != 2){
		LOG; ++fail;
	}
	if(Parser.bad_stream()){
		LOG; ++fail;
	}

	return fail;
}

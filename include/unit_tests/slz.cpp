//include
#include <slz/slz.hpp>
#include <unit_test.hpp>

int fail(0);

//one of every ASCII character
const std::string test_str(
	" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNO"
	"PQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"
);

class nested_message : public slz::message<0>
{
public:
	SLZ_CTOR_ASSIGN(nested_message)

	slz::ASCII<0> ASCII;
	slz::boolean<1> boolean;
	slz::string<2> string;
	slz::sint<3> sint;
	slz::uint<4> uint;
	slz::list<slz::ASCII<5> > ASCII_list;

	void set_test_vals()
	{
		ASCII = test_str;
		boolean = false;
		string = test_str;
		uint = 123;
		sint = -123;
		ASCII_list->push_back(test_str);
		ASCII_list->push_back(test_str);
	}

private:
	void add()
	{
		add_field(ASCII);
		add_field(boolean);
		add_field(string);
		add_field(sint);
		add_field(uint);
		add_field(ASCII_list);
	}

	void copy(const nested_message & rval)
	{
		ASCII = rval.ASCII;
		boolean = rval.boolean;
		string = rval.string;
		sint = rval.sint;
		uint = rval.uint;
		ASCII_list = rval.ASCII_list;
	}
};

class message : public slz::message<1>
{
public:
	SLZ_CTOR_ASSIGN(message)

	slz::ASCII<1> ASCII;
	slz::boolean<2> boolean;
	slz::string<3> string;
	slz::sint<4> sint;
	slz::uint<5> uint;
	slz::list<slz::ASCII<6> > ASCII_list;
	slz::list<nested_message> Nested_Message_list;

	void set_test_vals()
	{
		ASCII = test_str;
		boolean = true;
		string = test_str;
		uint = 123;
		sint = -123;
		ASCII_list->push_back(test_str);
		ASCII_list->push_back(test_str);
		nested_message tmp;
		tmp.set_test_vals();
		Nested_Message_list->push_back(tmp);
		tmp.uint = 321;
		Nested_Message_list->push_back(tmp);
	}

private:
	void add()
	{
		add_field(ASCII);
		add_field(boolean);
		add_field(string);
		add_field(sint);
		add_field(uint);
		add_field(ASCII_list);
		add_field(Nested_Message_list);
	}

	void copy(const message & rval)
	{
		ASCII = rval.ASCII;
		boolean = rval.boolean;
		string = rval.string;
		sint = rval.sint;
		uint = rval.uint;
		ASCII_list = rval.ASCII_list;
		Nested_Message_list = rval.Nested_Message_list;
	}
};

void ASCII_field()
{
	slz::ASCII<0> field = test_str, parsed_field;
	if(!parsed_field.parse(field.serialize())){
		LOG; ++fail;
	}
	if(!field || !parsed_field || *field != *parsed_field){
		LOG; ++fail;
	}
}

void boolean_field()
{
	slz::boolean<0> field = true, parsed_field;
	if(!parsed_field.parse(field.serialize())){
		LOG; ++fail;
	}
	if(!field || !parsed_field || *field != *parsed_field){
		LOG; ++fail;
	}
	field = false;
	if(!parsed_field.parse(field.serialize())){
		LOG; ++fail;
	}
	if(!field || !parsed_field || *field != *parsed_field){
		LOG; ++fail;
	}
}

void uint_field()
{
	slz::uint<0> field = 123, parsed_field;
	if(!parsed_field.parse(field.serialize())){
		LOG; ++fail;
	}
	if(!field || !parsed_field || *field != *parsed_field){
		LOG; ++fail;
	}
}

void sint_field()
{
	slz::sint<0> field = -123, parsed_field;
	if(!parsed_field.parse(field.serialize())){
		LOG; ++fail;
	}
	if(!field || !parsed_field || *field != *parsed_field){
		LOG; ++fail;
	}
}

void string_field()
{
	slz::string<0> field = test_str, parsed_field;
	if(!parsed_field.parse(field.serialize())){
		LOG; ++fail;
	}
	if(!field || !parsed_field || *field != *parsed_field){
		LOG; ++fail;
	}
}

void list_field()
{
	{//BEGIN scope
	//list of non-length_delimited values
	slz::list<slz::uint<0> > field, parsed_field;
	field->push_back(0);
	field->push_back(1);
	field->push_back(2);
	if(!parsed_field.parse(field.serialize())){
		LOG; ++fail;
	}
	if(*field != *parsed_field){
		LOG; ++fail;
	}
	}//END scope

	{//BEGIN scope
	//list of length_delimited values
	slz::list<slz::string<0> > field, parsed_field;
	field->push_back("ABCD");
	field->push_back("123");
	field->push_back("VWXYZ");
	if(!parsed_field.parse(field.serialize())){
		LOG; ++fail;
	}
	if(*field != *parsed_field){
		LOG; ++fail;
	}
	}//END scope

	{//BEGIN scope
	//list of lists
	slz::list<slz::list<slz::uint<0> > > field, parsed_field;
	slz::list<slz::uint<0> > field_element;
	field_element->push_back(0);
	field_element->push_back(1);
	field_element->push_back(2);
	field->push_back(field_element);
	field->push_back(field_element);
	field->push_back(field_element);
	if(!parsed_field.parse(field.serialize())){
		LOG; ++fail;
	}
	if(*field != *parsed_field){
		LOG; ++fail;
	}
	}//END scope
}

void message_field()
{
	message field, parsed_field;
	field.set_test_vals();
	if(!parsed_field.parse(field.serialize())){
		LOG; ++fail;
	}
	if(field != parsed_field){
		LOG; ++fail;
	}
}

void unknown_field()
{
	//old message version
	class m_old : public slz::message<0>
	{
	public:
		m_old()
		{
			add_field(string);
		}
		slz::string<1> string;
	} M_Old;

	//new message version that has additional field
	class m_new : public slz::message<0>
	{
	public:
		m_new()
		{
			add_field(string);
			add_field(uint);

			string = test_str;
			uint = 123;
		}

		slz::string<1> string;
		slz::uint<2> uint;
	} M_New;
	if(!M_Old.parse(M_New.serialize())){
		LOG; ++fail;
	}
	//M_Old holds unknown field
	if(M_Old != M_New){
		LOG; ++fail;
	}
	//equivalent to M_Old != M_New
	if(M_Old.serialize() != M_New.serialize()){
		LOG; ++fail;
	}
}

void parser()
{
	slz::parser Parser;
	Parser.add_field<message>();
	message field;
	field.set_test_vals();
	std::string buf;
	buf += field.serialize();
	buf += field.serialize();
	unsigned test_parsed = 0;
	while(boost::shared_ptr<slz::field> M = Parser.parse(buf)){
		if(typeid(*M) == typeid(message)){
			message * parsed_field = (message *)M.get();
			if(field != *parsed_field){
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

	//test slz::parser::missing
	buf = field.serialize();
	buf.erase(buf.size() - 5);
	boost::optional<unsigned> mis = Parser.missing(buf);
	if(!mis || *mis != 5){
		LOG; ++fail;
	}
}

int main()
{
	unit_test::timeout();

	ASCII_field();
	boolean_field();
	uint_field();
	sint_field();
	list_field();
	message_field();
	unknown_field();

	parser();

	return fail;
}

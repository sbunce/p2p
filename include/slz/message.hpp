#ifndef H_SLZ_MESSAGE
#define H_SLZ_MESSAGE

//custom
#include "field.hpp"

namespace slz{

//base class for user defined messages
class message
{
public:
	/*
	This virtual dtor makes the message type polymorphic. This is required so
	that typeid returns the derived type instead of the base type.
	*/
	virtual ~message(){}

	bool operator == (const message & rval)
	{
		//message must be composed of same fields
		if(Field.size() != rval.Field.size()){
			return false;
		}
		//state of fields must be equal
		for(std::map<boost::uint64_t, field *>::const_iterator
			it_cur = Field.begin(), rval_it_cur = rval.Field.begin(),
			it_end = Field.end(), rval_it_end = rval.Field.end();
			it_cur != it_end; ++it_cur, ++rval_it_cur)
		{
			if(*it_cur->second != *rval_it_cur->second){
				//one of the fields is set and one is not
				return false;
			}
			if(it_cur->second->serialize() != rval_it_cur->second->serialize()){
				//fields have different values
				return false;
			}
		}
		return true;
	}

	bool operator != (const message & rval)
	{
		return !(*this == rval);
	}

	//clear all fields
	void clear()
	{
		for(std::map<boost::uint64_t, field *>::iterator it_cur = Field.begin(),
			it_end = Field.end(); it_cur != it_end; ++it_cur)
		{
			it_cur->second->clear();
		}
	}

	//returns all field IDs in this message
	std::list<boost::uint64_t> ID()
	{
		std::list<boost::uint64_t> tmp;
		for(std::map<boost::uint64_t, field *>::iterator it_cur = Field.begin(),
			it_end = Field.end(); it_cur != it_end; ++it_cur)
		{
			tmp.push_back(it_cur->first);
		}
		return tmp;
	}

	/*
	Parse message. Returns false if field malformed. Fields cleared before
	parsing attempted.
	*/
	bool parse(const std::string & buf)
	{
		clear();
		std::list<std::string> fields = message_to_fields(buf);
		if(fields.empty()){
			return false;
		}
		for(std::list<std::string>::iterator it_cur = fields.begin(),
			it_end = fields.end(); it_cur != it_end; ++it_cur)
		{
			std::string key_vint = vint_parse(*it_cur);
			if(key_vint.empty() || buf.empty()){
				return false;
			}
			boost::uint64_t key = vint_to_uint(key_vint);
			boost::uint64_t field_UID = (key >> 1);
			std::map<boost::uint64_t, field *>::iterator it = Field.find(field_UID);
			if(it != Field.end()){
				if(!it->second->parse(*it_cur)){
					return false;
				}
			}
		}
		return true;
	}

	//returns serialized version of all fields
	std::string serialize()
	{
		std::string tmp;
		for(std::map<boost::uint64_t, field *>::iterator it_cur = Field.begin(),
			it_end = Field.end(); it_cur != it_end; ++it_cur)
		{
			tmp += it_cur->second->serialize();
		}
		tmp = uint_to_vint(tmp.size()) + tmp;
		return tmp;
	}

protected:
	//must be called for all fields
	void add_field(field & F)
	{
		Field.insert(std::make_pair(F.ID(), &F));
	}

private:
	//field ID mapped to field
	std::map<boost::uint64_t, field *> Field;
};

}//end of namespace slz
#endif

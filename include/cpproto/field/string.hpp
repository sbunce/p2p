#ifndef H_CPPROTO_FIELD_STRING
#define H_CPPROTO_FIELD_STRING

//custom
#include "../field.hpp"

namespace cpproto{
template<boost::uint64_t T_field_ID>
class string : public field
{
public:
	static const boost::uint64_t field_ID = T_field_ID;
	static const bool length_delim = true;

	string()
	{

	}

	string(const std::string & val_in):
		val(val_in)
	{

	}

	string(const char * val_in):
		val(val_in)
	{

	}

	string & operator = (const std::string & rval)
	{
		val = rval;
		return *this;
	}

	std::string & operator * ()
	{
		return val;
	}

	std::string & operator -> ()
	{
		return val;
	}

	virtual operator bool () const
	{
		return !val.empty();
	}

	virtual void clear()
	{
		val.clear();
	}

	virtual boost::uint64_t ID() const
	{
		return field_ID;
	}

	virtual bool parse(std::string buf)
	{
		clear();
		boost::optional<std::pair<boost::uint64_t, bool> > key = key_split(buf);
		if(!key || key->first != field_ID || key->second != true){
			return false;
		}
		std::string len_vint = vint_split(buf);
		if(len_vint.empty() || buf.empty()){
			return false;
		}
		boost::uint64_t len = vint_to_uint(len_vint);
		if(buf.size() != len){
			return false;
		}
		val = buf;
		return true;
	}

	virtual std::string serialize() const
	{
		if(val.empty()){
			return "";
		}
		std::string ser;
		ser += key_create(field_ID, true);
		ser += uint_to_vint(val.size());
		ser += val;
		return ser;
	}

private:
	std::string val;
};
}//end namespace cpproto
#endif

#ifndef H_SLZ_FIELD_STRING
#define H_SLZ_FIELD_STRING

namespace slz{

template<boost::uint64_t T_field_UID>
class string : public field
{
public:
	static const boost::uint64_t field_UID = T_field_UID;
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
		return field_UID;
	}

	virtual bool parse(std::string buf)
	{
		clear();
		std::string key_vint = vint_parse(buf);
		buf.erase(0, key_vint.size());
		if(key_vint.empty() || buf.empty()){
			return false;
		}
		boost::uint64_t key = vint_to_uint(key_vint);
		boost::uint64_t key_field_UID = (key >> 1);
		bool key_length_delim = (key & 1);
		if(key_field_UID != field_UID || key_length_delim != true){
			return false;
		}
		std::string len_vint = vint_parse(buf);
		buf.erase(0, len_vint.size());
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
		boost::uint64_t key = (field_UID << 1);
		key |= 1;
		std::string tmp;
		tmp += uint_to_vint(key);
		tmp += uint_to_vint(val.size()) + val;
		return tmp;
	}

private:
	std::string val;
};

}//end namespace slz
#endif

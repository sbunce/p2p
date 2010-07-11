#ifndef H_SLZ_FIELD_BOOLEAN
#define H_SLZ_FIELD_BOOLEAN

namespace slz{

template<boost::uint64_t T_field_UID>
class boolean : public field
{
public:
	static const boost::uint64_t field_UID = T_field_UID;
	static const bool length_delim = false;

	boolean()
	{

	}

	boolean(const bool val_in):
		val(val_in)
	{

	}

	boolean & operator = (const bool rval)
	{
		val = rval;
		return *this;
	}

	bool & operator * ()
	{
		return *val;
	}

	virtual operator bool () const
	{
		return val;
	}

	virtual void clear()
	{
		if(val){
			val.reset();
		}
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
		if(key_field_UID != field_UID || key_length_delim != false){
			return false;
		}
		std::string val_vint = vint_parse(buf);
		buf.erase(0, val_vint.size());
		if(val_vint.empty() || !buf.empty()){
			return false;
		}
		val = (vint_to_uint(val_vint) != 0);
		return true;
	}

	virtual std::string serialize() const
	{
		if(!val){
			return "";
		}
		boost::uint64_t key = (field_UID << 1);
		std::string tmp;
		tmp += uint_to_vint(key);
		tmp += uint_to_vint(*val);
		return tmp;
	}

private:
	boost::optional<bool> val;
};

}//end namespace slz
#endif

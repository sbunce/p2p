#ifndef H_SLZ_FIELD_SINT
#define H_SLZ_FIELD_SINT

namespace slz{

template<boost::uint64_t T_field_UID>
class sint : public field
{
public:
	static const boost::uint64_t field_UID = T_field_UID;
	static const bool length_delim = false;

	sint()
	{

	}

	sint(const boost::int64_t & val_in):
		val(val_in)
	{

	}

	sint & operator = (const boost::int64_t rval)
	{
		val = rval;
		return *this;
	}

	boost::int64_t & operator * ()
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
		val = decode_sint(vint_to_uint(val_vint));
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
		tmp += uint_to_vint(encode_sint(*val));
		return tmp;
	}

private:
	boost::optional<boost::int64_t> val;

	//encode signed as unsigned, needed to store signed int as vint
	boost::uint64_t encode_sint(const boost::int64_t x) const
	{
		return (x << 1) ^ (x >> 63);
	}

	//decode signed int encoded as unsigned
	boost::int64_t decode_sint(const boost::uint64_t x) const
	{
		return (x >> 1) ^ -static_cast<boost::int64_t>(x & 1);
	}
};

}//end namespace slz
#endif

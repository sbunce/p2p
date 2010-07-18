#ifndef H_SLZ_FIELD_SINT
#define H_SLZ_FIELD_SINT

//custom
#include "../field.hpp"

namespace slz{
template<boost::uint64_t T_field_ID>
class sint : public field
{
public:
	static const boost::uint64_t field_ID = T_field_ID;
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
		return field_ID;
	}

	virtual bool parse(std::string buf)
	{
		clear();
		boost::optional<std::pair<boost::uint64_t, bool> > key = key_split(buf);
		if(!key || key->first != field_ID || key->second != false){
			return false;
		}
		std::string val_vint = vint_split(buf);
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
		std::string ser;
		ser += key_create(field_ID, false);
		ser += uint_to_vint(encode_sint(*val));
		return ser;
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

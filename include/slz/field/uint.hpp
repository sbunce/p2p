#ifndef H_SLZ_FIELD_UINT
#define H_SLZ_FIELD_UINT

namespace slz{

template<boost::uint64_t T_field_UID>
class uint : public field
{
public:
	static const boost::uint64_t field_UID = T_field_UID;
	static const bool length_delim = false;

	uint()
	{

	}

	uint(const boost::uint64_t val_in):
		val(val_in)
	{

	}

	uint & operator = (const boost::uint64_t rval)
	{
		val = rval;
		return *this;
	}

	boost::uint64_t & operator * ()
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

	virtual bool parse(std::string buf)
	{
		clear();
		boost::optional<std::pair<boost::uint64_t, bool> > key = key_split(buf);
		if(!key || key->first != field_UID || key->second != false){
			return false;
		}
		std::string val_vint = vint_split(buf);
		if(val_vint.empty() || !buf.empty()){
			return false;
		}
		val = vint_to_uint(val_vint);
		return true;
	}

	virtual std::string serialize() const
	{
		if(!val){
			return "";
		}
		std::string ser;
		ser += key_create(field_UID, false);
		ser += uint_to_vint(*val);
		return ser;
	}

	virtual boost::uint64_t UID() const
	{
		return field_UID;
	}

private:
	boost::optional<boost::uint64_t> val;
};

}//end namespace slz
#endif
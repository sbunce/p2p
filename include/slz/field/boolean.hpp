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
		val = (vint_to_uint(val_vint) != 0);
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
	boost::optional<bool> val;
};

}//end namespace slz
#endif
#ifndef H_SLZ_FIELD_UNKNOWN
#define H_SLZ_FIELD_UNKNOWN

namespace slz{

//field we do not understand
class unknown : public field
{
public:
	virtual operator bool () const
	{
		return key_val.empty();
	}

	virtual void clear()
	{
		key_val.clear();
	}

	virtual boost::uint64_t ID() const
	{
		return field_UID;
	}

	virtual bool parse(std::string buf)
	{
		std::string key_vint = vint_parse(buf);
		if(key_vint.empty() || buf.size() <= key_vint.size()){
			return false;
		}
		boost::uint64_t key = vint_to_uint(key_vint);
		field_UID = (key >> 1);
		key_val = buf;
		return true;
	}

	virtual std::string serialize() const
	{
		return key_val;
	}

private:
	boost::uint64_t field_UID;
	std::string key_val;
};

}//end namespace slz
#endif

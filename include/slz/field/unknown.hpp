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

	virtual bool parse(std::string buf)
	{
		clear();
		boost::optional<std::pair<boost::uint64_t, bool> > key = key_parse(buf);
		if(!key){
			return false;
		}
		field_UID = key->first;
		key_val = buf;
		return true;
	}

	virtual std::string serialize() const
	{
		return key_val;
	}

	virtual boost::uint64_t UID() const
	{
		return field_UID;
	}

private:
	boost::uint64_t field_UID;
	std::string key_val;
};

}//end namespace slz
#endif

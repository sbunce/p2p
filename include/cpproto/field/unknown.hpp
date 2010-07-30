#ifndef H_CPPROTO_FIELD_UNKNOWN
#define H_CPPROTO_FIELD_UNKNOWN

//custom
#include "../field.hpp"

namespace cpproto{
//field we do not understand
class unknown : public field
{
public:
	virtual operator bool () const
	{
		return !key_val.empty();
	}

	virtual void clear()
	{
		key_val.clear();
	}

	virtual boost::uint64_t ID() const
	{
		return field_ID;
	}

	virtual bool parse(std::string buf)
	{
		clear();
		boost::optional<std::pair<boost::uint64_t, bool> > key = key_parse(buf);
		if(!key){
			return false;
		}
		field_ID = key->first;
		key_val = buf;
		return true;
	}

	virtual std::string serialize() const
	{
		return key_val;
	}

private:
	boost::uint64_t field_ID;
	std::string key_val;
};
}//end namespace cpproto
#endif

#ifndef H_SLZ_FIELD
#define H_SLZ_FIELD

//custom
#include "func.hpp"

//include
#include <boost/cstdint.hpp>

//standard
#include <set>

namespace slz{
//base class for all fields
class field
{
public:
	/*
	Derived must define these two data members for the vector field to work.
	static const boost::uint64_t field_UID = T_field_UID;
	static const bool length_delim = false;
	*/

	/*
	operator bool:
		True if field set (acts like boost::optional).
	clear:
		Unset field.
	parse:
		Parse field. Return false if field malformed. Field cleared before parse.
	serialize:
		Returns serialized version of field.
	UID:
		Unique ID for the field.
	*/
	virtual operator bool () const = 0;
	virtual void clear() = 0;
	virtual bool parse(std::string buf) = 0;
	virtual std::string serialize() const = 0;
	virtual boost::uint64_t UID() const = 0;

	bool operator == (const field & rval)
	{
		return serialize() == rval.serialize();
	}
	bool operator != (const field & rval)
	{
		return !(*this == rval);
	}
};
}//end namespace slz

#include "field/unknown.hpp"
#include "field/ASCII.hpp"
#include "field/boolean.hpp"
#include "field/list.hpp"
#include "field/message.hpp"
#include "field/sint.hpp"
#include "field/string.hpp"
#include "field/uint.hpp"


#endif

#ifndef H_CPPROTO_FIELD
#define H_CPPROTO_FIELD

//custom
#include "func.hpp"

//include
#include <boost/cstdint.hpp>
#include <boost/optional.hpp>
#include <logger.hpp>

//standard
#include <string>

namespace cpproto{
//base class for all fields
class field
{
public:
	/*
	Derived must define these two data members for the list field to work.
	static const boost::uint64_t field_ID; //key field_ID
	static const bool length_delim;        //true if field length delimited
	*/

	/*
	operator bool:
		True if field set (acts like boost::optional).
	clear:
		Unset field.
	ID:
		ID for the field.
	parse:
		Parse field. Return false if field malformed. Field cleared before parse.
	serialize:
		Returns serialized version of field.
	*/
	virtual operator bool () const = 0;
	virtual void clear() = 0;
	virtual boost::uint64_t ID() const = 0;
	virtual bool parse(std::string buf) = 0;
	virtual std::string serialize() const = 0;

	//operators
	bool operator == (const field & rval)
	{
		return serialize() == rval.serialize();
	}
	bool operator != (const field & rval)
	{
		return !(*this == rval);
	}
	bool operator < (const field & rval)
	{
		return serialize() < rval.serialize();
	}
};
}//end namespace cpproto
#endif

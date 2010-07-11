#ifndef H_SLZ_FIELD
#define H_SLZ_FIELD

//custom
#include "func.hpp"

//include
#include <boost/cstdint.hpp>

namespace slz{
//base class for all fields
class field
{
public:
	/*
	Derived classes must define the following data members.
	static const boost::uint64_t field_UID;
	static const bool length_delim = <true/false>;
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
	*/
	virtual operator bool () const = 0;
	virtual void clear() = 0;
	virtual boost::uint64_t ID() const = 0;
	virtual bool parse(std::string buf) = 0;
	virtual std::string serialize() const = 0;
};
}//end namespace slz

#include "field/ASCII.hpp"
#include "field/boolean.hpp"
#include "field/sint.hpp"
#include "field/string.hpp"
#include "field/uint.hpp"
#include "field/unknown.hpp"
#include "field/vector.hpp"

#endif

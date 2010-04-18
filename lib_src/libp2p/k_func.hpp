#ifndef H_K_FUNC
#define H_K_FUNC

//custom
#include "protocol_udp.hpp"

//include
#include <bit_field.hpp>
#include <convert.hpp>
#include <mpa.hpp>

namespace k_func
{
/*
distance:
	Returns the distance from one ID to another. Distance is symmetric so it
	doesn't matter what order the IDs are in.
ID_to_bit_field:
	Converts a ID to a bit_field.
ID_to_bucket_num:
	Returns what bucket a ID belongs in.
	Note: One of the IDs should be local, and one remote.
ID_to_mpint:
	Converts a ID to a mpint.
*/
extern mpa::mpint distance(const std::string & ID_0, const std::string & ID_1);
extern bit_field ID_to_bit_field(const std::string & ID);
extern unsigned ID_to_bucket_num(const std::string & ID_0,
	const std::string & ID_1);
extern mpa::mpint ID_to_mpint(const std::string & ID);
}//end of namespace k_func
#endif

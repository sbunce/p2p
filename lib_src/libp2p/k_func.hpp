#ifndef H_K_FUNC
#define H_K_FUNC

//custom
#include "protocol_udp.hpp"

//include
#include <bit_field.hpp>
#include <convert.hpp>

namespace k_func
{
/*
ID_to_bit_field:
	Converts a ID to a bit_field.
ID_to_bucket_num:
	Returns what bucket a ID belongs in.
	Note: One of the IDs should be local, and one remote.
*/
extern "C++" {
bit_field ID_to_bit_field(const std::string & ID);
unsigned ID_to_bucket_num(const std::string & ID_0,
	const std::string & ID_1);
}//end of extern
}//end of namespace k_func
#endif

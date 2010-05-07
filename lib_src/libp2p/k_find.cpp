#include "k_find.hpp"

k_find::k_find(
	const std::string & ID_to_find_in,
	const boost::function<void (const net::endpoint)> & call_back_in
):
	ID_to_find(ID_to_find_in),
	call_back(call_back_in)
{

}

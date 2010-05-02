#ifndef H_NETWORK_ADDRESS_IMPL
#define H_NETWORK_ADDRESS_IMPL

//custom
#include "system_include.hpp"

namespace network{
class address_impl
{
public:
	addrinfo ai;
	sockaddr_storage sas;
};
}//end namespace network
#endif

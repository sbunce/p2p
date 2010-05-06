#include "addr_impl.hpp"

net::addr_impl::addr_impl(const addrinfo * ai_in)
{
	copy(ai_in);
}

net::addr_impl::addr_impl(const addr_impl & AI)
{
	copy(&AI.ai);
}

net::addr_impl & net::addr_impl::operator = (const addr_impl & AI)
{
	copy(&AI.ai);
	return *this;
}

void net::addr_impl::copy(const addrinfo * ai_in)
{
	assert(ai_in != NULL);
	//shallow copy
	ai = *ai_in;

	//change pointer to sas in this object
	ai.ai_addr = reinterpret_cast<sockaddr *>(&sas);

	//copy address
	std::memcpy(ai.ai_addr, ai_in->ai_addr, ai_in->ai_addrlen);

	//we do not save full host name
	ai.ai_canonname = NULL;

	//not part of a list
	ai.ai_next = NULL;
}

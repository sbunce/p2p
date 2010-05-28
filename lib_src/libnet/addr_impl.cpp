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
	ai = *ai_in;
	ai.ai_addr = reinterpret_cast<sockaddr *>(&sas);
	std::memcpy(ai.ai_addr, ai_in->ai_addr, ai_in->ai_addrlen);
	ai.ai_canonname = NULL;
	ai.ai_next = NULL;
}

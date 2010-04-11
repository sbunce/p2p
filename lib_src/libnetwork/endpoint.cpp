#include <network/network.hpp>

network::endpoint::endpoint(const addrinfo * ai_in)
{
	assert(ai_in != NULL);
	copy(ai_in);
}

network::endpoint::endpoint(const std::string & addr, const std::string & port,
	const socket_t type)
{
	assert(addr.size() == 4 || addr.size() == 16);
	assert(port.size() == 2);

	//create endpoint
	std::set<endpoint> E = get_endpoint(addr.size() == 4 ? "127.0.0.1" : "::1", "0", type);
	assert(!E.empty());
	copy(&E.begin()->ai);

	//replace localhost address and port
	if(ai.ai_addr->sa_family == AF_INET){
		assert(addr.size() == 4);
		reinterpret_cast<sockaddr_in *>(ai.ai_addr)->sin_addr.s_addr
			= convert::bin_to_int<boost::uint32_t>(addr);
		reinterpret_cast<sockaddr_in *>(ai.ai_addr)->sin_port
			= convert::bin_to_int<boost::uint16_t>(port);
	}else if(ai.ai_addr->sa_family == AF_INET6){
		assert(addr.size() == 16);
		std::memcpy(reinterpret_cast<sockaddr_in6 *>(ai.ai_addr)->sin6_addr.s6_addr,
			addr.data(), addr.size());
		reinterpret_cast<sockaddr_in6 *>(ai.ai_addr)->sin6_port
			= convert::bin_to_int<boost::uint16_t>(port);
	}else{
		LOGGER << "unknown address family";
		exit(1);
	}
}

network::endpoint::endpoint(const endpoint & E)
{
	copy(&E.ai);
}

std::string network::endpoint::IP() const
{
	char buf[INET6_ADDRSTRLEN];
	if(getnameinfo(ai.ai_addr, ai.ai_addrlen, buf, sizeof(buf), NULL, 0,
		NI_NUMERICHOST) == -1)
	{
		return "";
	}
	return buf;
}

std::string network::endpoint::IP_bin() const
{
	if(ai.ai_addr->sa_family == AF_INET){
		return convert::int_to_bin(reinterpret_cast<sockaddr_in *>(ai.ai_addr)->sin_addr.s_addr);
	}else if(ai.ai_addr->sa_family == AF_INET6){
		return std::string(reinterpret_cast<const char *>(
			reinterpret_cast<sockaddr_in6 *>(ai.ai_addr)->sin6_addr.s6_addr), 16);
	}else{
		LOGGER << "unknown address family";
		exit(1);
	}
}

std::string network::endpoint::port() const
{
	char buf[6];
	if(getnameinfo(ai.ai_addr, ai.ai_addrlen, NULL, 0, buf, sizeof(buf),
		NI_NUMERICSERV) == -1)
	{
		return "";
	}
	return buf;
}

std::string network::endpoint::port_bin() const
{
	if(ai.ai_addr->sa_family == AF_INET){
		return convert::int_to_bin(reinterpret_cast<sockaddr_in *>(ai.ai_addr)->sin_port);
	}else if(ai.ai_addr->sa_family == AF_INET6){
		return convert::int_to_bin(reinterpret_cast<sockaddr_in6 *>(ai.ai_addr)->sin6_port);
	}else{
		LOGGER << "unknown address family";
		exit(1);
	}
}

network::socket_t network::endpoint::type() const
{
	if(ai.ai_socktype == SOCK_STREAM){
		return tcp;
	}else{
		return udp;
	}
}

network::endpoint & network::endpoint::operator = (const endpoint & rval)
{
	copy(&rval.ai);
	return *this;
}

bool network::endpoint::operator < (const endpoint & rval) const
{
	return IP() + port() < rval.IP() + rval.port();
}

bool network::endpoint::operator == (const endpoint & rval) const
{
	return !(*this < rval) && !(rval < *this);
}

bool network::endpoint::operator != (const endpoint & rval) const
{
	return !(*this == rval);
}

void network::endpoint::copy(const addrinfo * ai_in)
{
	ai = *ai_in;
	ai.ai_addr = reinterpret_cast<sockaddr *>(&sas);
	std::memcpy(ai.ai_addr, ai_in->ai_addr, ai_in->ai_addrlen);
	ai.ai_canonname = NULL;
	ai.ai_next = NULL;
}

std::set<network::endpoint> network::get_endpoint(const std::string & host,
	const std::string & port, const socket_t type)
{
	std::set<endpoint> E;
	if(port.empty() || host.size() > 255 || port.size() > 33){
		return E;
	}
	addrinfo hints;
	std::memset(&hints, 0, sizeof(addrinfo));
	hints.ai_family = AF_UNSPEC;
	if(type == tcp){
		hints.ai_socktype = SOCK_STREAM;
	}else{
		hints.ai_socktype = SOCK_DGRAM;
	}
	if(host.empty()){
		/*
		Generally used when getting endpoint information for listener and we want
		to listen on all interfaces.
		*/
		hints.ai_flags = AI_PASSIVE;
	}
	addrinfo * result;
	if(getaddrinfo(host.empty() ? NULL : host.c_str(), port.c_str(), &hints, &result) == 0){
		for(addrinfo * cur = result; cur != NULL; cur = cur->ai_next){			
			E.insert(endpoint(cur));
		}
		freeaddrinfo(result);
	}
	return E;
}

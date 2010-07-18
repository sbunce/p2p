#include <net/endpoint.hpp>

net::endpoint::endpoint(const addrinfo * ai_in)
{
	net::init::start();
	copy(ai_in);
}

net::endpoint::endpoint(const endpoint & E)
{
	net::init::start();
	copy(&E.ai);
}

net::endpoint::~endpoint()
{
	net::init::stop();
}

void net::endpoint::copy(const addrinfo * ai_in)
{
	assert(ai_in != NULL);
	ai = *ai_in;
	ai.ai_addr = reinterpret_cast<sockaddr *>(&sas);
	std::memcpy(ai.ai_addr, ai_in->ai_addr, ai_in->ai_addrlen);
	ai.ai_canonname = NULL;
	ai.ai_next = NULL;
}

std::string net::endpoint::IP() const
{
	char buf[INET6_ADDRSTRLEN];
	if(getnameinfo(ai.ai_addr, ai.ai_addrlen, buf, sizeof(buf), NULL, 0,
		NI_NUMERICHOST) == -1)
	{
		return "";
	}
	return buf;
}

std::string net::endpoint::IP_bin() const
{
	if(ai.ai_addr->sa_family == AF_INET){
		return convert::int_to_bin(reinterpret_cast<sockaddr_in *>(ai.ai_addr)->sin_addr.s_addr);
	}else if(ai.ai_addr->sa_family == AF_INET6){
		return std::string(reinterpret_cast<const char *>(
			reinterpret_cast<sockaddr_in6 *>(ai.ai_addr)->sin6_addr.s6_addr), 16);
	}else{
		LOG << "unknown address family";
		exit(1);
	}
}

std::string net::endpoint::port() const
{
	char buf[6];
	if(getnameinfo(ai.ai_addr, ai.ai_addrlen, NULL, 0, buf, sizeof(buf),
		NI_NUMERICSERV) == -1)
	{
		return "";
	}
	return buf;
}

std::string net::endpoint::port_bin() const
{
	if(ai.ai_addr->sa_family == AF_INET){
		return std::string(reinterpret_cast<const char *>(
			&reinterpret_cast<sockaddr_in *>(ai.ai_addr)->sin_port), 2);
	}else if(ai.ai_addr->sa_family == AF_INET6){
		return std::string(reinterpret_cast<const char *>(
			&reinterpret_cast<sockaddr_in6 *>(ai.ai_addr)->sin6_port), 2);
	}else{
		LOG << "unknown address family";
		exit(1);
	}
}

net::version_t net::endpoint::version() const
{
	if(ai.ai_addr->sa_family == AF_INET){
		return IPv4;
	}else{
		return IPv6;
	}
}

net::endpoint & net::endpoint::operator = (const endpoint & rval)
{
	copy(&rval.ai);
	return *this;
}

bool net::endpoint::operator < (const endpoint & rval) const
{
	return IP() + port() < rval.IP() + rval.port();
}

bool net::endpoint::operator == (const endpoint & rval) const
{
	return !(*this < rval) && !(rval < *this);
}

bool net::endpoint::operator != (const endpoint & rval) const
{
	return !(*this == rval);
}

std::set<net::endpoint> net::get_endpoint(const std::string & host,
	const std::string & port)
{
	net::init Init;
	std::set<endpoint> E;
	if(port.empty() || host.size() > 255 || port.size() > 33){
		return E;
	}
	/*
	The ai_socktype and ai_protocol members are always set by the class which
	takes the endpoint as a paramter. For example when you call nstream::open
	ai_socktype is set to SOCK_STREAM and ai_protocol is set to IPPROTO_TCP.
	*/
	addrinfo hints;
	std::memset(&hints, 0, sizeof(addrinfo));
	hints.ai_family = AF_UNSPEC; //IPv4 or IPv6
	if(host.empty()){
		/*
		Generally used when getting endpoint information for listener and we want
		to listen on all interfaces.
		*/
		hints.ai_flags = AI_PASSIVE;
	}
	int err;
	addrinfo * result;
	if((err = getaddrinfo(host.empty() ? NULL : host.c_str(), port.c_str(),
		&hints, &result)) == 0)
	{
		for(addrinfo * cur = result; cur != NULL; cur = cur->ai_next){
			E.insert(endpoint(cur));
		}
		freeaddrinfo(result);
	}else{
		LOG << "\"" << host << "\" \"" << gai_strerror(err) << "\"";
	}
	return E;
}

boost::optional<net::endpoint> net::bin_to_endpoint(const std::string & addr,
	const std::string & port)
{
	net::init Init;
	assert(addr.size() == 4 || addr.size() == 16);
	assert(port.size() == 2);

	//create endpoint
	std::set<net::endpoint> ep_set = get_endpoint(addr.size() == 4 ? "127.0.0.1" : "::1", "0");
	if(ep_set.empty()){
		//this can happen on a IPv6 address when host doesn't support IPv6
		return boost::optional<net::endpoint>();
	}
	boost::optional<net::endpoint> ep(*ep_set.begin());

	//replace localhost address and port
	if(ep->ai.ai_addr->sa_family == AF_INET){
		assert(addr.size() == 4);
		reinterpret_cast<sockaddr_in *>(ep->ai.ai_addr)->sin_addr.s_addr
			= convert::bin_to_int<boost::uint32_t>(addr);
		std::memcpy(reinterpret_cast<void *>(&reinterpret_cast<sockaddr_in *>(ep->ai.ai_addr)->sin_port),
			reinterpret_cast<const void *>(port.data()), 2);
	}else if(ep->ai.ai_addr->sa_family == AF_INET6){
		assert(addr.size() == 16);
		std::memcpy(reinterpret_cast<sockaddr_in6 *>(ep->ai.ai_addr)->sin6_addr.s6_addr,
			addr.data(), addr.size());
		std::memcpy(reinterpret_cast<void *>(&reinterpret_cast<sockaddr_in6 *>(ep->ai.ai_addr)->sin6_port),
			reinterpret_cast<const void *>(port.data()), 2);
	}else{
		LOG << "unknown address family";
		exit(1);
	}
	return ep;
}

#ifndef H_NETWORK_ENDPOINT
#define H_NETWORK_ENDPOINT

//custom
#include "protocol.hpp"
#include "system_include.hpp"

//include
#include <logger.hpp>

//standard
#include <set>

namespace network{
class listener;
class ndgram;
class nstream;
class endpoint
{
	friend class listener;
	friend class ndgram;
	friend class nstream;
public:
	explicit endpoint(const addrinfo * ai_in)
	{
		copy(ai_in);
	}

	endpoint(const endpoint & E)
	{
		copy(&E.ai);
	}

	std::string IP() const
	{
		char buf[INET6_ADDRSTRLEN];
		if(getnameinfo(ai.ai_addr, ai.ai_addrlen, buf, sizeof(buf), NULL, 0,
			NI_NUMERICHOST) == -1)
		{
			return "";
		}
		return buf;
	}

	std::string port() const
	{
		char buf[6];
		if(getnameinfo(ai.ai_addr, ai.ai_addrlen, NULL, 0, buf, sizeof(buf),
			NI_NUMERICSERV) == -1)
		{
			return "";
		}
		return buf;
	}

	sock_type type() const
	{
		if(ai.ai_socktype == SOCK_STREAM){
			return tcp;
		}else{
			return udp;
		}
	}

	endpoint & operator = (const endpoint & rval)
	{
		copy(&rval.ai);
		return *this;
	}

	bool operator < (const endpoint & rval) const
	{
		return (IP() + port()) < (rval.IP() + rval.port());
	}

private:
	//wrapped info needed to connect to a host
	addrinfo ai;
	sockaddr_storage sas;

	void copy(const addrinfo * ai_in)
	{
		ai = *ai_in;
		ai.ai_addr = reinterpret_cast<sockaddr *>(&sas);
		std::memcpy(ai.ai_addr, ai_in->ai_addr, ai_in->ai_addrlen);
		ai.ai_canonname = NULL;
		ai.ai_next = NULL;
	}
};

/*
Returns set of possible endpoints for the specified host and port. The
std::set is used because the getaddrinfo function can return duplicates.
*/
static std::set<endpoint> get_endpoint(const std::string & host,
	const std::string & port, const sock_type type)
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
}
#endif

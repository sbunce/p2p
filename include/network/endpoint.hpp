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

//classes which need access to private addrinfo
class listener;
class ndgram;
class nstream;

class endpoint
{
	friend class listener;
	friend class ndgram;
	friend class nstream;
	friend std::set<endpoint> get_endpoint(const std::string & host,
	const std::string & port, const socket_t type);

public:
	endpoint(const endpoint & E);

	/*
	IP:
		Returns IP address.
	port:
		Returns port.
	type:
		Returns type. (tcp or udp)
	*/
	std::string IP() const;
	std::string port() const;
	socket_t type() const;

	//operators
	endpoint & operator = (const endpoint & rval);
	bool operator < (const endpoint & rval) const;
	bool operator == (const endpoint & rval) const;
	bool operator != (const endpoint & rval) const;

private:
	explicit endpoint(const addrinfo * ai_in);

	//wrapped info needed to connect to a host
	addrinfo ai;
	sockaddr_storage sas;

	void copy(const addrinfo * ai_in);
};

/*
Returns set of possible endpoints for the specified host and port. The
std::set is used because the getaddrinfo function can return duplicates.
*/
extern std::set<endpoint> get_endpoint(const std::string & host,
	const std::string & port, const socket_t type);
}//end of namespace network
#endif

#ifndef H_NET_ENDPOINT
#define H_NET_ENDPOINT

//custom
#include "init.hpp"
#include "protocol.hpp"

//include
#include <boost/optional.hpp>
#include <convert.hpp>
#include <logger.hpp>
#include <portable.hpp>

//standard
#include <set>

namespace net{

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
		const std::string & port);
	friend boost::optional<net::endpoint> bin_to_endpoint(
		const std::string & addr, const std::string & port);
public:
	endpoint(const endpoint & E);
	~endpoint();

	/*
	IP:
		Returns IP address.
	IP_bin:
		Returns binary IP address in big-endian. This will be 4 bytes for IPv4 and
		16 bytes for IPv6.
	port:
		Returns port.
	port_bin:
		Returns binary port in big-endian. This will always be 2 bytes.
	version:
		IPv4 or IPv6.
	*/
	std::string IP() const;
	std::string IP_bin() const;
	std::string port() const;
	std::string port_bin() const;
	version_t version() const;

	//operators
	endpoint & operator = (const endpoint & rval);
	bool operator < (const endpoint & rval) const;
	bool operator == (const endpoint & rval) const;
	bool operator != (const endpoint & rval) const;

private:
	explicit endpoint(const addrinfo * ai_in);

	addrinfo ai;
	sockaddr_storage sas;

	/*
	copy:
		Used to copy endpoint addrinfo.
	*/
	void copy(const addrinfo * ai_in);
};

/*
Returns set of possible endpoints for the specified host and port. The
std::set is used because the getaddrinfo function can return duplicates.
*/
extern std::set<endpoint> get_endpoint(const std::string & host,
	const std::string & port);

/*
Converts bytes to endpoint. A IPv6 endpoint won't be returned if the system
doesn't support IPv6.
Note: addr must be 4 or 16 bytes. port must be 2 bytes.
*/
extern boost::optional<net::endpoint> bin_to_endpoint(
	const std::string & addr, const std::string & port);
}//end of namespace net
#endif

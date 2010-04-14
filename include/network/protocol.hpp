#ifndef H_NETWORK_PROTOCOL
#define H_NETWORK_PROTOCOL

namespace network{
enum direction_t{
	incoming, //we accepted connection
	outgoing  //we initiated connect
};
enum socket_t{
	tcp,
	udp
};
enum version_t{
	IPv4,
	IPv6
};
}
#endif

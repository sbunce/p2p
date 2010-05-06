#ifndef H_NET_PROTOCOL
#define H_NET_PROTOCOL

namespace net{
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
}//end of namespace net
#endif

#ifndef H_NETWORK_PROTOCOL
#define H_NETWORK_PROTOCOL

namespace network{
enum socket_t{
	tcp,
	udp
};
enum direction_t{
	incoming, //we accepted connection
	outgoing  //we initiated connect
};
}
#endif

#ifndef H_NETWORK_PROTOCOL
#define H_NETWORK_PROTOCOL

namespace network{
enum sock_type{
	tcp,
	udp
};
enum dir{
	incoming, //we accepted connection
	outgoing  //we initiated connect
};
}
#endif

#ifndef H_MESSAGE_UDP
#define H_MESSAGE_UDP

//include
#include <network/network.hpp>

namespace message_udp {
namespace recv{

class base
{
public:
	/*
	expect:
		Returns true if message expected.
	recv:
		Returns true if incoming message received. False if we don't expect the
		message.
		Note: It is not necessary to call expect() before this function.
	*/
	virtual bool expect(network::buffer & recv_buf) = 0;
	virtual bool recv(network::buffer & recv_buf) = 0;
};

}//end of namespace recv

namespace send{

}//end of namespace send
}//end of namespace message_udp
#endif

#ifndef H_MESSAGE_UDP
#define H_MESSAGE_UDP

//include
#include <network/network.hpp>

namespace message_udp {

//abstract base class for all messages
class base
{
public:
	//function to handle incoming message (empty if message to send)
	boost::function<bool (boost::shared_ptr<base>)> func;

	//bytes that have been sent or received
	network::buffer buf;

	/*
	expects:
		Returns true if recv() expects message on front of CI.recv_buf.
	recv:
		Returns true if incoming message received. False if incomplete message or
		host blacklisted.
	*/
	virtual bool expects(network::buffer & recv_buf) = 0;
	virtual bool recv(network::buffer & recv_buf) = 0;
};

}//end of namespace message_udp
#endif

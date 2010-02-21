#ifndef H_EXCHANGE_UDP
#define H_EXCHANGE_UDP

//custom
#include "message_udp.hpp"

//include
#include <boost/utility.hpp>

class exchange_udp : private boost::noncopyable
{
public:
	exchange_udp();

	/*
	recv_call_back:
		Called when message received.
	*/
	//void recv_call_back(network::buffer & recv_buf, boost::shared_ptr<network::endpoint> & from);

	/*

	*/
/*
	void expect_anytime(boost::shared_ptr<message_tcp::base> M);
	void expect_anytime_erase(network::buffer buf);
	void send(boost::shared_ptr<message_tcp::base> M);
*/
private:

};
#endif

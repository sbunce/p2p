#ifndef H_CONNECTION
#define H_CONNECTION

//custom
#include "encryption.hpp"
#include "exchange.hpp"
#include "message.hpp"
#include "share.hpp"
#include "slot_manager.hpp"

//include
#include <boost/utility.hpp>
#include <network/network.hpp>

class connection : private boost::noncopyable
{
public:
	connection(
		network::proactor & Proactor_in,
		network::connection_info & CI
	);

private:
	//locks all entry points to this object
	boost::recursive_mutex Recursive_Mutex;

	exchange Exchange;         //used to exchange messages
	slot_manager Slot_Manager; //does everything slot related

	/*
	recv_call_back:
		Proactor calls back to this function whenever data is received.
	recv_initial:
		Call back for receiving initial message.
	send_initial:
		Send initial message. Called after key exchange completed.
	*/
	void recv_call_back(network::connection_info & CI);
	bool recv_initial(boost::shared_ptr<message::base> M);
	void send_initial();
};
#endif

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

	/*
	empty:
		Returns true if no slots are open and not slots need to be opened.
	remove:
		Close and remove all outgoing slots with the specified hash.
	*/
	bool empty();
	void remove(const std::string & hash);

private:
	//locks all entry points to this object
	boost::mutex Mutex;

	exchange Exchange;         //used to exchange messages
	slot_manager Slot_Manager; //does everything slot related

	/*
	exchange_call_back:
		Called after exchange done processing buffers. Does periodic tasks.
	recv_initial:
		Call back for receiving initial message.
	send_initial:
		Send initial message. Called after key exchange completed.
	*/
	void exchange_call_back();
	bool recv_initial(boost::shared_ptr<message::base> M);
	void send_initial();
};
#endif

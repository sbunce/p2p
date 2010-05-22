#ifndef H_CONNECTION
#define H_CONNECTION

//custom
#include "encryption.hpp"
#include "exchange_tcp.hpp"
#include "message_tcp.hpp"
#include "share.hpp"
#include "slot_manager.hpp"

//include
#include <boost/utility.hpp>
#include <net/net.hpp>

class connection : private boost::noncopyable
{
public:
	connection(
		net::proactor & Proactor_in,
		net::proactor::connection_info & CI,
		const boost::function<void(const int)> & trigger_tick
	);

	/*
	empty:
		Returns true if no slots are open and no slots need to be opened.
	remove:
		Close and remove all outgoing slots with the specified hash.
	tick:
		Do periodic tasks.
	*/
//DEBUG, consider add() function to open slots.
//Then keep track of pair<IP,port> to hash mappings in connection_manager.

	bool empty();
	void remove(const std::string & hash);
	void tick();

private:
	//locks all entry points to this object
	boost::mutex Mutex;

	exchange_tcp Exchange;
	slot_manager Slot_Manager;

	/*
	recv_call_back:
		Called by connection::recv_call_back, which is called by proactor.
	send_call_back:
		Called by connection::send_call_back, which is called by proactor.
	*/
	void recv_call_back(net::proactor::connection_info & CI);
	void send_call_back(net::proactor::connection_info & CI);

	/*
	recv_initial:
		Call back for receiving initial message.
	send_initial:
		Send initial message. Called after key exchange completed.
	*/
	bool recv_initial(const std::string & ID);
	void send_initial();
};
#endif

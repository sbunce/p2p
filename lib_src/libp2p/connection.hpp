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
		const boost::function<void(const net::endpoint & ep, const std::string & hash)> & peer_call_back,
		const boost::function<void(const int)> & trigger_tick
	);

	/*
	add:
		Add hash of a file to download.
	empty:
		Returns true if no slots are open and no slots need to be opened.
	remove:
		Close and remove all outgoing slots with the specified hash.
	tick:
		Do periodic tasks.
	*/
	void add(const std::string & hash);
	bool empty();
	void remove(const std::string & hash);
	void tick();

private:
	//lock for all entry points to this object
	boost::mutex Mutex;

	exchange_tcp Exchange;
	slot_manager Slot_Manager;

	/*
	The endpoint of the remote host.
	Note: This is not the endpoint that we can connect to because the port is
		randomly assigned by the remote host. The remote host will tell us the
		correct port in the initial message.
	*/
	const net::endpoint remote_ep;

	/*
	recv_call_back:
		Called by connection::recv_call_back, which is called by proactor.
	send_call_back:
		Called by connection::send_call_back, which is called by proactor.
	*/
	void recv_call_back(net::proactor::connection_info & CI);
	void send_call_back(net::proactor::connection_info & CI);

	/*
	recv_initial_ID:
		Called when we receive remote_ID.
	recv_initial_port:
		Called when we receive port.
		Note: Only called on incoming connections.
	*/
	bool recv_initial_ID(const std::string & remote_ID);
	bool recv_initial_port(const std::string & port);
};
#endif

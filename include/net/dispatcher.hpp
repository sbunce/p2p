#ifndef H_NET_DISPATCHER
#define H_NET_DISPATCHER

//custom
#include "connection_info.hpp"

//include
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <thread_pool.hpp>

//standard
#include <list>

namespace net{
class dispatcher : private boost::noncopyable
{
public:
	dispatcher(
		const boost::function<void (connection_info &)> & connect_call_back_in,
		const boost::function<void (connection_info &)> & disconnect_call_back_in
	);
	~dispatcher();

	/*
	join:
		Block until all dispatches complete.
	*/
	void join();

	/*
	connect:
		Schedule call back for connect.
		Precondition: No other call back must have been scheduled before the
			connect call back.
	disconnect:
		Schedule call back for disconnect, or failed to connect. If scheduling a
		job for a disconnect then a connect call back must have been scheduled
		first.
		Postcondition: No other jobs for the connection should be scheduled.
	recv:
		Schedule call back for recv.
		Precondition: The connect call back must have been scheduled.
	send:
		Schedule call back for send_buf size decrease.
		Precondition: The connect call back must have been scheduled.
	*/
	void connect(const boost::shared_ptr<connection_info> & CI);
	void disconnect(const boost::shared_ptr<connection_info> & CI);
	void recv(const boost::shared_ptr<connection_info> & CI,
		const boost::shared_ptr<buffer> & recv_buf);
	void send(const boost::shared_ptr<connection_info> & CI, const unsigned latest_send,
		const unsigned send_buf_size);

private:
	const boost::function<void (connection_info &)> connect_call_back;
	const boost::function<void (connection_info &)> disconnect_call_back;

	/*
	Invariants to the call back system:
	1. Only one call back for a socket is done concurrently.
	2. The first call back for a socket must be the connect call back.
	3. The last call back for a socket must be the disconnect call back.

	The memoize system takes care of invariant 1.
	Invariant 2 is insured by always pushing connect jobs on the the front of the
		job container.
	Invariant 3 is insured by requiring that disconnect jobs be scheduled last.
	*/
	boost::mutex Mutex;    //locks everything in this section
	std::list<std::pair<int, boost::function<void ()> > > Jobs;
	std::set<int> Memoize; //used to memoize connection_ID

	/*
	dispatch:
		Worker threads which do call backs reside in this function.
		Precondition: Mutex locked.
	*/
	void dispatch();

	/*
	connect_call_back_wrapper:
		Wraps connect call back. Used to dereference shared_ptr.
	disconnect_call_back_wrapper:
		Wraps disconnect call back. Used to dereference shared_ptr.
	recv_call_back_wrapper:
		Wrapper for recv_call_back to dereference shared_ptr. Also, this function is
		necessary to allow recv_call_back to be changed while another recv call back
		is being blocked by in dispatch().
	send_call_back_wrapper:
		Wrapper for send_call_back to dereference shared_ptr. Also, this function is
		necessary to allow send_call_back to be changed while another send call back
		is being blocked by in dispatch().
	*/
	void connect_call_back_wrapper(boost::shared_ptr<connection_info> CI);
	void disconnect_call_back_wrapper(boost::shared_ptr<connection_info> CI);
	void recv_call_back_wrapper(boost::shared_ptr<connection_info> CI,
		boost::shared_ptr<buffer> recv_buf);
	void send_call_back_wrapper(boost::shared_ptr<connection_info> CI,
		const unsigned latest_send, const int send_buf_size);

	thread_pool Thread_Pool;
};
}//end of namespace net
#endif

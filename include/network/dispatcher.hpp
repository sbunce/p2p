#ifndef H_NETWORK_DISPATCHER
#define H_NETWORK_DISPATCHER

//custom
#include "connection_info.hpp"

//include
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

//standard
#include <list>

namespace network{
class dispatcher : private boost::noncopyable
{
	//number of threads to use to do call backs
	static const unsigned dispatcher_threads = 4;
public:
	dispatcher(
		const boost::function<void (connection_info &)> & connect_call_back_in,
		const boost::function<void (connection_info &)> & disconnect_call_back_in
	);
	~dispatcher();

	/*
	start:
		Allow the dispatcher to accept new jobs.
	stop_join:
		Stop dispatcher from accepting new jobs. Block until all existing jobs are
		finished.
	*/
	void start();
	void stop_join();

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
	//threads to do call backs
	boost::thread_group workers;

	const boost::function<void (connection_info &)> connect_call_back;
	const boost::function<void (connection_info &)> disconnect_call_back;

	/*
	Call backs to be done by dispatch(). The job container holds all call backs
	which need to be made. The stopped and dtor_stopped flags are used for
	assertions to insure the dispatcher is not incorrectly used.

	There are some invariants to the call back system.
	1. Only one call back for a socket is done concurrently.
	2. The first call back for a socket must be the connect call back.
	3. The last call back for a socket must be the disconnect call back.

	The memoize system takes care of invariant 1.
	Invariant 2 is insured by always pushing connect jobs on the the front of the
		job container.
	Invariant 3 is insured by requiring that disconnect jobs be scheduled last.
	*/
	boost::mutex job_mutex; //locks everything in this section
	boost::condition_variable_any job_cond;  //cond to check for new jobs
	boost::condition_variable_any join_cond; //cond used for join
	std::list<std::pair<int, boost::function<void ()> > > job_list;
	std::set<int> memoize;  //used to memoize connection_ID
	unsigned running_jobs;  //number of jobs in progress
	bool stopped;           //if true worker terminates when no more jobs

	/*
	dispatch:
		Worker threads which do call backs reside in this function.
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
};
}
#endif

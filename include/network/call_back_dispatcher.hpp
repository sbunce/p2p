#ifndef H_NETWORK_CALL_BACK_DISPATCHER
#define H_NETWORK_CALL_BACK_DISPATCHER

//custom
#include "connection_info.hpp"

//include
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

//standard
#include <deque>

namespace network{
class call_back_dispatcher : private boost::noncopyable
{
public:

	call_back_dispatcher(
		const boost::function<void (connection_info &)> & connect_call_back_in,
		const boost::function<void (connection_info &)> & disconnect_call_back_in
	):
		connect_call_back(connect_call_back_in),
		disconnect_call_back(disconnect_call_back_in)
	{
		int threads = boost::thread::hardware_concurrency() < 8 ?
			8 : boost::thread::hardware_concurrency();
		for(int x=0; x<threads; ++x){
			Workers.create_thread(boost::bind(&call_back_dispatcher::dispatch, this));
		}
	}

	~call_back_dispatcher()
	{
		Workers.interrupt_all();
		Workers.join_all();
	}

	/*
	Schedule call back for connect.
	Precondition: No other call back must have been scheduled before the connect
		call back.
	*/
	void connect(const boost::shared_ptr<connection_info> & CI)
	{
		boost::mutex::scoped_lock lock(job_mutex);
		job.push_front(std::make_pair(CI->connection_ID, boost::bind(
			&call_back_dispatcher::connect_call_back_wrapper, this, CI)));
		job_cond.notify_one();
	}

	/*
	Schedule call back for recv.
	Precondition: The connect call back must have been scheduled.
	*/
	void recv(const boost::shared_ptr<connection_info> & CI,
		const boost::shared_ptr<buffer> & recv_buf)
	{
		boost::mutex::scoped_lock lock(job_mutex);
		job.push_back(std::make_pair(CI->connection_ID, boost::bind(
			&call_back_dispatcher::recv_call_back_wrapper, this, CI, recv_buf)));
		job_cond.notify_one();
	}

	/*
	Schedule call back for send_buf size decrease.
	Precondition: The connect call back must have been scheduled.
	*/
	void send(const boost::shared_ptr<connection_info> & CI, const int send_buf_size)
	{
		boost::mutex::scoped_lock lock(job_mutex);
		job.push_back(std::make_pair(CI->connection_ID, boost::bind(
			&call_back_dispatcher::send_call_back_wrapper, this, CI, send_buf_size)));
		job_cond.notify_one();
	}

	/*
	Schedule call back for disconnect, or failed to connect. If scheduling a job
	for a disconnect then a connect call back must have been scheduled first.
	Postcondition: No other jobs for the connection should be scheduled.
	*/
	void disconnect(const boost::shared_ptr<connection_info> & CI)
	{
		boost::mutex::scoped_lock lock(job_mutex);
		job.push_back(std::make_pair(CI->connection_ID, boost::bind(
			&call_back_dispatcher::disconnect_call_back_wrapper, this, CI)));
		job_cond.notify_one();
	}

private:

	//threads which wait in call_back_dispatch
	boost::thread_group Workers;

	const boost::function<void (connection_info &)> connect_call_back;
	const boost::function<void (connection_info &)> disconnect_call_back;

	/*
	Used by call_back_dispatch to memoize the connection_ID for each call back in
	progress so that it can stop multiple call backs for the same socket from
	occuring concurrently.
	Note: memoize_mutex must lock all access to container.
	Note: memoize_cond notified when call back completed.
	*/
	boost::mutex memoize_mutex;
	boost::condition_variable_any memoize_cond;
	std::set<int> memoize;

	/*
	Call backs to be done by dispatch(). The connect_job container holds connect
	call back jobs. The other_job container holds send call back and disconnect
	call back jobs.

	There are some invariants to the call back system.
	1. Only one call back for a socket is done concurrently.
	2. The first call back for a socket must be the connect call back.
	3. The last call back for a socket must be the disconnect call back.

	The memoize system takes care of invariant 1.
	Invariant 2 is insured by always pushing connect jobs on the the front of the
		job container.
	Invariant 3 is insured by requiring that disconnect jobs be scheduled last.
	*/
	boost::mutex job_mutex;
	boost::condition_variable_any job_cond;
	std::deque<std::pair<int, boost::function<void ()> > > job;

	//worker threads which do call backs reside in this function
	void dispatch()
	{
		std::pair<int, boost::function<void ()> > temp;
		while(true){
			{//begin lock scope
			boost::mutex::scoped_lock lock(job_mutex);
			while(job.empty()){
				job_cond.wait(job_mutex);
			}
			temp = job.front();
			job.pop_front();
			}//end lock scope

			{//begin lock scope
			boost::mutex::scoped_lock lock(memoize_mutex);
			while(true){
				std::pair<std::set<int>::iterator, bool> ret = memoize.insert(temp.first);
				if(ret.second){
					break;
				}else{
					//call back for socket is currently running
					memoize_cond.wait(memoize_mutex);
				}
			}
			}//end lock scope

			//run call back
			temp.second();

			{//begin lock scope
			boost::mutex::scoped_lock lock(memoize_mutex);
			memoize.erase(temp.first);
			}//end lock scope
			memoize_cond.notify_all();
		}
	}

	//wrapper for connect call back to dereference shared_ptr
	void connect_call_back_wrapper(boost::shared_ptr<connection_info> CI)
	{
		connect_call_back(*CI);
	}

	//wrapper for connect call back to dereference shared_ptr
	void disconnect_call_back_wrapper(boost::shared_ptr<connection_info> CI)
	{
		disconnect_call_back(*CI);
	}

	/*
	Wrapper for recv_call_back to dereference shared_ptr. Also, this function is
	necessary to allow recv_call_back to be changed while another recv call back
	is being blocked by in dispatch().
	*/
	void recv_call_back_wrapper(boost::shared_ptr<connection_info> CI, boost::shared_ptr<buffer> recv_buf)
	{
		if(CI->recv_call_back){
			CI->recv_call_back(*CI, *recv_buf);
		}
	}

	/*
	Wrapper for send_call_back to dereference shared_ptr. Also, this function is
	necessary to allow send_call_back to be changed while another send call back
	is being blocked by in dispatch().
	*/
	void send_call_back_wrapper(boost::shared_ptr<connection_info> CI, const int send_buf_size)
	{
		if(CI->send_call_back){
			CI->send_call_back(*CI, send_buf_size);
		}
	}
};
}
#endif

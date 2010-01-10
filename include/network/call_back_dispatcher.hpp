#ifndef H_NETWORK_CALL_BACK_DISPATCHER
#define H_NETWORK_CALL_BACK_DISPATCHER

//custom
#include "connection_info.hpp"

//include
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

//standard
#include <list>

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
	Starts threads to do call backs.
	Precondition: Call_Back_Dispatcher must be stopped.
	*/
	void start()
	{
		boost::mutex::scoped_lock lock(start_stop_mutex);
		assert(!Workers); //assert stopped
		Workers = boost::shared_ptr<boost::thread_group>(new boost::thread_group());
		int threads = boost::thread::hardware_concurrency() < 4 ?
			4 : boost::thread::hardware_concurrency();
		for(int x=0; x<threads; ++x){
			Workers->create_thread(boost::bind(&call_back_dispatcher::dispatch, this));
		}
	}

	/*
	Stops all call back threads. Cancels all pending jobs.
	Precondition: Call_Back_Dispatcher must be started.
	*/
	void stop()
	{
		boost::mutex::scoped_lock lock(start_stop_mutex);
		assert(Workers); //assert started
		Workers->interrupt_all();
		Workers->join_all();
		Workers = boost::shared_ptr<boost::thread_group>();
		job.clear();
		memoize.clear();
	}

private:
	boost::mutex start_stop_mutex;

	//threads which wait in call_back_dispatch
	boost::shared_ptr<boost::thread_group> Workers;

	const boost::function<void (connection_info &)> connect_call_back;
	const boost::function<void (connection_info &)> disconnect_call_back;

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
	boost::mutex job_mutex; //locks everything in this section
	boost::condition_variable_any job_cond;
	std::list<std::pair<int, boost::function<void ()> > > job;
	std::set<int> memoize;  //used to memoize connection_ID

	//worker threads which do call backs reside in this function
	void dispatch()
	{
		std::pair<int, boost::function<void ()> > temp;
		while(true){
			{//begin lock scope
			boost::mutex::scoped_lock lock(job_mutex);
			while(true){
				for(std::list<std::pair<int, boost::function<void ()> > >::iterator
					iter_cur = job.begin(), iter_end = job.end(); iter_cur != iter_end;
					++iter_cur)
				{
					std::pair<std::set<int>::iterator, bool>
						ret = memoize.insert(iter_cur->first);
					if(ret.second){
						temp = *iter_cur;
						job.erase(iter_cur);
						goto run_dispatch;
					}
				}
				job_cond.wait(job_mutex);
			}
			}//end lock scope
			run_dispatch:

			//run call back
			temp.second();

			{//begin lock scope
			boost::mutex::scoped_lock lock(job_mutex);
			memoize.erase(temp.first);
			}//end lock scope
			job_cond.notify_all();
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
			CI->recv_buf.append(*recv_buf);
			CI->latest_recv = recv_buf->size();
			CI->recv_call_back(*CI);
		}
	}

	/*
	Wrapper for send_call_back to dereference shared_ptr. Also, this function is
	necessary to allow send_call_back to be changed while another send call back
	is being blocked by in dispatch().
	*/
	void send_call_back_wrapper(boost::shared_ptr<connection_info> CI, const int send_buf_size)
	{
		CI->send_buf_size = send_buf_size;
		if(CI->send_call_back){
			CI->send_call_back(*CI);
		}
	}
};
}
#endif

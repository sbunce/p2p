#ifndef H_NETWORK_REACTOR
#define H_NETWORK_REACTOR

//custom
#include "rate_limit.hpp"
#include "sock.hpp"
#include "wrapper.hpp"

//include
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

namespace network{
class reactor : private boost::noncopyable
{
public:
	reactor()
	{
		wrapper::start_networking();
	}

	virtual ~reactor()
	{
		wrapper::stop_networking();
	}

	/*
	max_connections_supported:
		Maximum number of connections the reactor supports.
	start:
		Start the reactor.
	stop:
		SCRAM the reactor.
	*/
	virtual unsigned max_connections_supported() = 0;
	virtual void start() = 0;
	virtual void stop() = 0;

	//asynchronously connect, returns immediately
	void connect(boost::shared_ptr<sock> & S)
	{
		boost::mutex::scoped_lock lock(job_connect_mutex);
		if(S->socket_FD == -1 || !S->info->resolved()){
			S->failed_connect_flag = true;
			add_job(S);
		}else{
			job_connect.push_back(S);
			trigger_selfpipe();
		}
	}

	//block until a some socket_data needs attention
	boost::shared_ptr<sock> get()
	{
		boost::mutex::scoped_lock lock(job_mutex);
		while(job.empty()){
			job_cond.wait(job_mutex);
		}
		boost::shared_ptr<sock> S = job.front();
		job.pop_front();
		return S;
	}

	//when finished with a socket return it here
	void put(boost::shared_ptr<sock> & S)
	{
		if(!S->disconnect_flag && !S->failed_connect_flag){
			boost::mutex::scoped_lock lock(job_finished_mutex);

			//after the first get() of this sock the connect job is done
			const_cast<bool &>(S->connect_flag) = true;

			//reset other flags
			S->recv_flag = false;
			S->send_flag = false;

			job_finished.push_back(S);
			trigger_selfpipe();
		}
	}

	//functions for getting/setting rate limits, and getting current rates
	unsigned current_download_rate()
	{
		return Rate_Limit.current_download_rate();
	}
	unsigned current_upload_rate()
	{
		return Rate_Limit.current_upload_rate();
	}
	unsigned get_max_download_rate()
	{
		return Rate_Limit.get_max_download_rate();
	}
	unsigned get_max_upload_rate()
	{
		return Rate_Limit.get_max_upload_rate();
	}
	void set_max_download_rate(const unsigned rate)
	{
		Rate_Limit.set_max_download_rate(rate);
	}
	void set_max_upload_rate(const unsigned rate)
	{
		Rate_Limit.set_max_upload_rate(rate);
	}

protected:
	//controls maximum upload/download rate
	rate_limit Rate_Limit;

	/*
	trigger_selfpipe:
		See selfpipe documentation in reactor_select.
	*/
	virtual void trigger_selfpipe() = 0;

	//add a job that a thread calling get() can pick up
	void add_job(boost::shared_ptr<sock> & S)
	{
		boost::mutex::scoped_lock lock(job_mutex);
		job.push_back(S);
		job_cond.notify_one();
	}

	//get sock that needs to connect, empty shared_ptr returned if none
	boost::shared_ptr<sock> get_job_connect()
	{
		boost::mutex::scoped_lock lock(job_connect_mutex);
		if(job_connect.empty()){
			return boost::shared_ptr<sock>();
		}else{
			boost::shared_ptr<sock> S = job_connect.front();
			job_connect.pop_front();
			return S;
		}
	}

	//get sock that was returned with put(), empty shared_ptr returned if none
	boost::shared_ptr<sock> get_job_finished()
	{
		boost::mutex::scoped_lock lock(job_finished_mutex);
		if(job_finished.empty()){
			return boost::shared_ptr<sock>();
		}else{
			boost::shared_ptr<sock> S = job_finished.front();
			job_finished.pop_front();
			return S;
		}
	}

private:
	/*
	The connect() function puts sock's in the connect_job container to hand them
	off to the main_loop_thread which will do an asynchronous connection.
	*/
	boost::mutex job_connect_mutex;
	std::deque<boost::shared_ptr<sock> > job_connect;

	/*
	When a sock needs to be passed back to the client (because it has a flag set)
	it is put in the job container and job_cond is notified.
	*/
	boost::mutex job_mutex;
	boost::condition_variable_any job_cond;
	std::deque<boost::shared_ptr<sock> > job;

	/*
	This job_finished container is used by put() to pass a sock back to the
	main_loop_thread which will start monitoring it for activity.
	*/
	boost::mutex job_finished_mutex;
	std::deque<boost::shared_ptr<sock> > job_finished;
};
}//end namespace network
#endif

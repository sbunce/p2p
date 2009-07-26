/*
This class has a dedicated thread which does DNS lookups. After it does the
lookup it will pass the constructed sock to the reactor passed in the ctor. If
a IP address is passed to this function it will be passed to the reactor
without delay.
*/
#ifndef H_NETWORK_ASYNC_RESOLVE
#define H_NETWORK_ASYNC_RESOLVE

//custom
#include "reactor.hpp"
#include "sock.hpp"
#include "wrapper.hpp"

//include
#include <boost/thread.hpp>
#include <logger.hpp>

//standard
#include <deque>

namespace network {
class async_resolve
{
public:
	async_resolve(reactor & Reactor_in):
		Reactor(Reactor_in)
	{}

	void connect(const std::string & host, const std::string & port)
	{
		boost::mutex::scoped_lock lock(job_mutex);
		job.push_back(std::make_pair(host, port));
		job_cond.notify_one();
	}

	void start()
	{
		boost::mutex::scoped_lock lock(start_stop_mutex);
		if(main_loop_thread.get_id() != boost::thread::id()){
			LOGGER << "start called on already started async_resolve";
			exit(1);
		}
		main_loop_thread = boost::thread(boost::bind(&async_resolve::main_loop, this));
	}

	void stop()
	{
		boost::mutex::scoped_lock lock(start_stop_mutex);
		if(main_loop_thread.get_id() == boost::thread::id()){
			LOGGER << "stop called on already stopped async_resolve";
			exit(1);
		}
		main_loop_thread.interrupt();
		main_loop_thread.join();
	}

private:
	//mutex for start/stop
	boost::mutex start_stop_mutex;

	//thread for main_loop
	boost::thread main_loop_thread;

	reactor & Reactor;

	/*
	Resolve jobs queue'd by resolve().
	Note: job_mutex locks all access to job container.
	Note: job_cond is used to notify resolve thread when there is work.
	std::pair<host, IP>
	*/
	boost::mutex job_mutex;
	boost::condition_variable_any job_cond;
	std::deque<std::pair<std::string, std::string> > job;

	void main_loop()
	{
		while(true){
			std::pair<std::string, std::string> info;
			{//begin lock scope
			boost::mutex::scoped_lock lock(job_mutex);
			while(job.empty()){
				job_cond.wait(job_mutex);
			}
			info = job.front();
			job.pop_front();
			}//end lock scope
			boost::shared_ptr<address_info> AI(new address_info(
				info.first.c_str(), info.second.c_str()));
			boost::shared_ptr<sock> S(new sock(AI));
			Reactor.connect(S);
		}
	}
};
}//end of namespace network
#endif

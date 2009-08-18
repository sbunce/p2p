/*
The proactor gets jobs from a reactor and dispacthes them by calling the
appropriate call back. The proactor uses as many threads as there are CPUs.
*/
#ifndef H_NETWORK_PROACTOR
#define H_NETWORK_PROACTOR

//custom
#include "reactor_select.hpp"

//include
#include <boost/utility.hpp>
#include <logger.hpp>

namespace network{
class proactor
{
public:
	proactor(
		const std::string & port,
		boost::function<void (sock & Sock)> connect_call_back_in,
		boost::function<void (sock & Sock)> disconnect_call_back_in,
		boost::function<void (sock & Sock)> failed_connect_call_back_in
	):
		Reactor(port),
		connect_call_back(connect_call_back_in),
		disconnect_call_back(disconnect_call_back_in),
		failed_connect_call_back(failed_connect_call_back_in)
	{
		//start reactor that proactor uses
		Reactor.start();

		//start threads for doing call backs
		for(int x=0; x<boost::thread::hardware_concurrency(); ++x){
			Workers.create_thread(boost::bind(&proactor::dispatch, this));
		}

		//start thread for DNS resolution
		Workers.create_thread(boost::bind(&proactor::resolve, this));
	}

	~proactor()
	{
		Reactor.stop();
		Workers.interrupt_all();
		Workers.join_all();
	}

	/*
	Reactor which the proactor uses. This object is public and can be used to get
	network information, set rate limits, etc.
	*/
	reactor_select Reactor;

	/*
	Does asynchronous DNS resolution. Hands off DNS resolution to a thread inside
	the proactor which does DNS resolution, then calls reactor::connect.
	*/
	void connect(const std::string & host, const std::string & port)
	{
		boost::mutex::scoped_lock lock(resolve_job_mutex);
		resolve_job.push_back(std::make_pair(host, port));
		resolve_job_cond.notify_one();
	}

private:
	//mutex for start/stop
	boost::mutex start_stop_mutex;

	//worker threads to handle call backs
	boost::thread_group Workers;

	//call backs used by dispatch thread
	boost::function<void (sock & Sock)> failed_connect_call_back;
	boost::function<void (sock & Sock)> connect_call_back;
	boost::function<void (sock & Sock)> disconnect_call_back;

	/*
	Resolve jobs queue'd by resolve().
	Note: job_mutex locks all access to job container.
	Note: job_cond is used to notify resolve thread when there is work.
	std::pair<host, IP>
	*/
	boost::mutex resolve_job_mutex;
	boost::condition_variable_any resolve_job_cond;
	std::deque<std::pair<std::string, std::string> > resolve_job;

	//threads which do call backs reside in this function
	void dispatch()
	{
		while(true){
			boost::shared_ptr<sock> S = Reactor.get();
			if(S->failed_connect_flag){
				failed_connect_call_back(*S);
			}else{
				if(!S->connect_flag){
					connect_call_back(*S);
				}
				if(!S->disconnect_flag && S->recv_flag){
					assert(S->recv_call_back);
					S->recv_call_back(*S);
				}
				if(!S->disconnect_flag && S->send_flag){
					assert(S->send_call_back);
					S->send_call_back(*S);
				}
				if(S->disconnect_flag){
					disconnect_call_back(*S);
				}
			}
			Reactor.put(S);
		}
	}

	//thread which does DNS resolution resides in this function
	void resolve()
	{
		while(true){
			std::pair<std::string, std::string> job;
			{//begin lock scope
			boost::mutex::scoped_lock lock(resolve_job_mutex);
			while(resolve_job.empty()){
				resolve_job_cond.wait(resolve_job_mutex);
			}
			job = resolve_job.front();
			resolve_job.pop_front();
			}//end lock scope
			boost::shared_ptr<address_info> AI(new address_info(
				job.first.c_str(), job.second.c_str()));
			boost::shared_ptr<sock> S(new sock(AI));
			Reactor.connect(S);
		}
	}
};
}
#endif

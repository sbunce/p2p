/*
The proactor gets jobs from a reactor and dispacthes them by calling the
appropriate call back. The proactor uses as many threads as there are CPUs.
*/
#ifndef H_NETWORK_PROACTOR
#define H_NETWORK_PROACTOR

//custom
#include "buffer.hpp"
#include "reactor.hpp"
#include "sock.hpp"

//include
#include <boost/utility.hpp>
#include <logger.hpp>

//standard
#include <limits>

namespace network{
class proactor
{
public:
	proactor(
		reactor & Reactor_in,
		boost::function<void (sock & Sock)> connect_call_back_in,
		boost::function<void (sock & Sock)> disconnect_call_back_in,
		boost::function<void (sock & Sock)> failed_connect_call_back_in
	):
		Reactor(Reactor_in),
		connect_call_back(connect_call_back_in),
		disconnect_call_back(disconnect_call_back_in),
		failed_connect_call_back(failed_connect_call_back_in)
	{}

	//starts both the proactor
	void start()
	{
		boost::mutex::scoped_lock lock(start_stop_mutex);
		if(Workers.size() != 0){
			LOGGER << "start called on already started proactor";
			exit(1);
		}
		for(int x=0; x<boost::thread::hardware_concurrency(); ++x){
			Workers.create_thread(boost::bind(&proactor::dispatch, this));
		}
	}

	//must be called before destruction to stop threads
	void stop()
	{
		boost::mutex::scoped_lock lock(start_stop_mutex);
		if(Workers.size() == 0){
			LOGGER << "stop called on already stopped proactor";
			exit(1);
		}
		Workers.interrupt_all();
		Workers.join_all();
	}

private:
	//mutex for start/stop
	boost::mutex start_stop_mutex;

	reactor & Reactor;

	boost::function<void (sock & Sock)> failed_connect_call_back;
	boost::function<void (sock & Sock)> connect_call_back;
	boost::function<void (sock & Sock)> disconnect_call_back;

	//worker threads to handle call backs
	boost::thread_group Workers;

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
};
}
#endif

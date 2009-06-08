#ifndef H_NETWORK_REACTOR
#define H_NETWORK_REACTOR

//boost
#include <boost/utility.hpp>

//custom
#include "rate_limit.hpp"
#include "socket_data.hpp"
#include "wrapper.hpp"

//include
#include <buffer.hpp>
#include <logger.hpp>

namespace network{
class reactor
{
public:
	reactor(
		boost::function<void (socket_data::socket_data_visible &)> failed_connect_call_back_in,
		boost::function<void (socket_data::socket_data_visible &)> connect_call_back_in,
		boost::function<void (socket_data::socket_data_visible &)> disconnect_call_back_in
	):
		failed_connect_call_back(failed_connect_call_back_in),
		connect_call_back(connect_call_back_in),
		disconnect_call_back(disconnect_call_back_in)
	{
		wrapper::start_networking();

		for(int x=0; x<8; ++x){
			Workers.create_thread(boost::bind(&reactor::pool, this));
		}
	}

	virtual ~reactor()
	{
		Workers.interrupt_all();
		Workers.join_all();
		wrapper::stop_networking();
	}

	/*
	connect:
		Establishes new connection.
//DEBUG, have return value for maximum connections reached?
	get_job:
		Blocks until a job is ready. The job type is stored in socket_data
		Socket_State variable. This function is intended to be called by multiple
		worker threads to wait for jobs and do call backs.
	finish_job:
		After worker does call backs this function will be called so that the
		socket is again monitored for activity.
	*/
	virtual void connect(const std::string & host, const std::string & port) = 0;
	virtual boost::shared_ptr<socket_data> get_job() = 0;
	virtual void finish_job(boost::shared_ptr<socket_data> & info) = 0;

protected:
	/*
	Everything here is accessed by the reactor_* threads. These variables should
	not be read or modified by a thread coming in to the reactor. Instead 
	*/

	//controls maximum upload/download rate
	rate_limit Rate_Limit;

	/*
	Whenever a job needs to be done job_cond.notify_one() should be called to
	release a worker thread to check for jobs.
	*/
	boost::condition_variable_any worker_cond;

private:
	//worker threads to handle call backs
	boost::thread_group Workers;

	//mutex used with worker_cond
	boost::mutex worker_mutex;

	//call backs
	boost::function<void (socket_data::socket_data_visible &)> failed_connect_call_back;
	boost::function<void (socket_data::socket_data_visible &)> connect_call_back;
	boost::function<void (socket_data::socket_data_visible &)> disconnect_call_back;

	void pool()
	{
		while(true){
			//workers get blocked here while there is no work
			{//begin lock scope
			boost::mutex::scoped_lock lock(worker_mutex);
			worker_cond.wait(worker_mutex);
			}//end lock scope

			//check for call back job
			boost::shared_ptr<socket_data> info = get_job();
			if(info.get() != NULL){
				call_back(info);
				finish_job(info);
			}
		}
	}

	void call_back(boost::shared_ptr<socket_data> info)
	{
		if(info->failed_connect_flag){
			failed_connect_call_back(info->Socket_Data_Visible);
		}else{
			if(!info->connect_flag){
				info->connect_flag = true;
				connect_call_back(info->Socket_Data_Visible);
			}
			assert(info->Socket_Data_Visible.recv_call_back);
			assert(info->Socket_Data_Visible.send_call_back);
			if(info->recv_flag){
				info->recv_flag = false;
				info->Socket_Data_Visible.recv_call_back(info->Socket_Data_Visible);
			}
			if(info->send_flag){
				info->send_flag = false;
				info->Socket_Data_Visible.send_call_back(info->Socket_Data_Visible);
			}
			if(info->disconnect_flag){
				disconnect_call_back(info->Socket_Data_Visible);
			}
		}
	}
};
}
#endif

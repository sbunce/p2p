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
	//derived class ctors must call this base class ctor
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
	}

	virtual ~reactor()
	{
		Workers.interrupt_all();
		Workers.join_all();
		wrapper::stop_networking();
	}

	/*
	This function does asynchronous DNS resolution. It schedules a job and a
	worker does the resolution. After the resolution is complete a connection is
	made.
	*/
	void connect(const std::string & host, const std::string & port)
	{
		boost::mutex::scoped_lock lock(job_mutex);
		connect_job.push_back(std::make_pair(host, port));
		job_cond.notify_one();
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

	/*
	Starts workers which do call backs and DNS resolution. This is called by the
	derived class's start() function.
	*/
	void start()
	{
		//workers to do call backs and DNS resolution
		for(int x=0; x<8; ++x){
			Workers.create_thread(boost::bind(&reactor::pool, this));
		}
		start_networking();
	}

protected:
	//controls maximum upload/download rate
	rate_limit Rate_Limit;

	void add_call_back_job(boost::shared_ptr<socket_data> & info)
	{
		boost::mutex::scoped_lock lock(job_mutex);
		call_back_job.push_back(info);
		job_cond.notify_one();
	}

	/* Functions Reactors Must Define.
	connect:
		Establishes new connection. This is called by a worker after DNS
		resolution.
	finish_job:
		After worker does call backs this function will be called so that the
		socket is again monitored for activity.
	start_networking:
		After the reactor is constructed the reactor::start() function will be
		called. The reactor::start() function will call the start_networking()
		function to brind up the networking thread.
	*/
	virtual void connect(const std::string & host, const std::string & port,
		boost::shared_ptr<wrapper::info> Info) = 0;
	virtual void finish_job(boost::shared_ptr<socket_data> & info) = 0;
	virtual void start_networking() = 0;

private:
	//worker threads to handle call backs
	boost::thread_group Workers;

	/*
	Mutex used with job_cond. Mutex locks access to all job containers.
	*/
	boost::mutex job_mutex;
	boost::condition_variable_any job_cond;
	std::deque<std::pair<std::string, std::string> > connect_job; //std::pair<host, port>
	std::deque<boost::shared_ptr<socket_data> > call_back_job;

	//call backs
	boost::function<void (socket_data::socket_data_visible &)> failed_connect_call_back;
	boost::function<void (socket_data::socket_data_visible &)> connect_call_back;
	boost::function<void (socket_data::socket_data_visible &)> disconnect_call_back;

	void pool()
	{
		while(true){
			std::pair<std::string, std::string> connect_info;
			boost::shared_ptr<socket_data> call_back_info;

			{//begin lock scope
			boost::mutex::scoped_lock lock(job_mutex);
			if(connect_job.empty() && call_back_job.empty()){
				job_cond.wait(job_mutex);
			}

			//connect jobs always prioritized over call
			if(!connect_job.empty()){
				connect_info = connect_job.front();
				connect_job.pop_front();
			}else if(!call_back_job.empty()){
				call_back_info = call_back_job.front();
				call_back_job.pop_front();
			}
			}//end lock scope

			if(!connect_info.first.empty()){
				//construction of wrapper::info does DNS lookup
				boost::shared_ptr<wrapper::info> Info(new wrapper::info(
					connect_info.first.c_str(), connect_info.second.c_str()));
				connect(connect_info.first, connect_info.second, Info);
			}
			if(call_back_info.get() != NULL){
				run_call_back_job(call_back_info);
			}
		}
	}

	//do pending call backs
	void run_call_back_job(boost::shared_ptr<socket_data> & info)
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
				wrapper::disconnect(info->socket_FD);
				disconnect_call_back(info->Socket_Data_Visible);
			}
		}
		finish_job(info);
	}
};
}
#endif

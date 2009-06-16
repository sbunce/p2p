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
		boost::function<void (const std::string & host, const std::string & port)> failed_connect_call_back_in,
		boost::function<void (socket_data::socket_data_visible & Socket)> connect_call_back_in,
		boost::function<void (socket_data::socket_data_visible & Socket)> disconnect_call_back_in
	):
		failed_connect_call_back(failed_connect_call_back_in),
		connect_call_back(connect_call_back_in),
		disconnect_call_back(disconnect_call_back_in),
		max_connections(0),
		connections(0),
		incoming_connections(0),
		outgoing_connections(0)
	{
		wrapper::start_networking();
	}

	virtual ~reactor()
	{
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

	//functions for getting/settings max connections, and getting current connections
	unsigned get_connections()
	{
		boost::mutex::scoped_lock lock(connections_mutex);
		return connections;
	}
	unsigned get_incoming_connections()
	{
		boost::mutex::scoped_lock lock(connections_mutex);
		return incoming_connections;
	}
	unsigned get_outgoing_connections()
	{
		boost::mutex::scoped_lock lock(connections_mutex);
		return outgoing_connections;
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

	//must be called after construction to start reactor threads
	void start()
	{
		//workers to do call backs and DNS resolution
		for(int x=0; x<8; ++x){
			Workers.create_thread(boost::bind(&reactor::pool, this));
		}
		//thread to send/recv and create jobs for the workers
		start_networking();
	}

	//must be called before destruction to stop threads
	void stop()
	{
		stop_networking();
		Workers.interrupt_all();
		Workers.join_all();
	}

protected:
	//controls maximum upload/download rate
	rate_limit Rate_Limit;

	/*
	All these must be locked with connections_mutex. The max_connections are set
	in the ctor of the derived class.
	*/
	boost::mutex connections_mutex;
	unsigned max_connections;
	unsigned connections;
	unsigned incoming_connections;
	unsigned outgoing_connections;

	/*
	Adds a new socket_data for a specified socket.
	Precondition: socket_data must not already exist.
	Precondition: SD->socket_FD != -1.
	*/
	void add_socket_data(boost::shared_ptr<socket_data> & SD)
	{
		boost::mutex::scoped_lock lock(Socket_Data_mutex);
		if(SD->socket_FD == -1){
			LOGGER << "violated precondition";
			exit(1);
		}
		std::pair<std::map<int, boost::shared_ptr<socket_data> >::iterator, bool> ret;
		ret = Socket_Data_All.insert(std::make_pair(SD->socket_FD, SD));
		if(!ret.second){
			LOGGER << "violated precondition";
			exit(1);
		}
		ret = Socket_Data_Network.insert(std::make_pair(SD->socket_FD, SD));
		if(!ret.second){
			LOGGER << "violated precondition";
			exit(1);
		}
	}

	/*
	Called by the derived class to schedule a call back.
	Precondition: Network thread must have ownership of the socket_data (it must
		exist in Socket_Data_Network).
	Postcondition: This function disallows access to the socket_data to the
		network thread until the worker passes this socket_data to the finish_job
		function. If get_socket_data() is called when the socket_data is not in
		Socket_Data_Network the program will be terminated because this means a
		worker is using the socket_data.
	Note: After this function is called the network thread should not continue to
		use the socket_data.
	*/
	void call_back(const int socket_FD)
	{
		boost::shared_ptr<socket_data> SD;

		//find socket_data to do call back with
		{//begin lock scope
		boost::mutex::scoped_lock lock(Socket_Data_mutex);
		std::map<int, boost::shared_ptr<socket_data> >::iterator
			iter = Socket_Data_Network.find(socket_FD);
		if(iter == Socket_Data_Network.end()){
			LOGGER << "violated precondition";
			exit(1);
		}else{
			SD = iter->second;
			Socket_Data_Network.erase(iter);
		}
		}//end lock scope

		//schedule call back
		{//begin lock scope
		boost::mutex::scoped_lock lock(job_mutex);
		call_back_job.push_back(SD);
		job_cond.notify_one();
		}//end lock scope
	}

	/*
	This is a special call back to be used when a connection fails before there is
	any socket or socket_data. This will need to be used when DNS resolution or
	socket allocation fails.
	*/
	void call_back_failed_connect(const std::string & host, const std::string & port)
	{
		boost::mutex::scoped_lock lock(job_mutex);
		/*
		This is a dummy socket_data which allows us to use the same mechanism for doing
		the failed_connect_call_back(). It won't be passed back to finish_job().
		*/
		boost::shared_ptr<socket_data> SD(new socket_data(-1, host, "", port));
		SD->failed_connect_flag = true;
		call_back_job.push_back(SD);
		job_cond.notify_one();
	}

	/*
	Checks timeouts on all sockets.
	Note: Only sockets in Socket_Data_Network are checked because that means the
		workers aren't currently using them. A socket can only time out when the
		network thread isn't doing anything with it (no data to send/recv).
	Note: This function only needs to be checked once per second.
	*/
	void check_timeouts(std::vector<int> & timed_out)
	{
		boost::mutex::scoped_lock lock(Socket_Data_mutex);
		timed_out.clear();
		std::map<int, boost::shared_ptr<socket_data> >::iterator
		iter_cur = Socket_Data_Network.begin(),
		iter_end = Socket_Data_Network.end();
		while(iter_cur != iter_end){
			if(std::time(NULL) - iter_cur->second->last_seen > 4){
				timed_out.push_back(iter_cur->second->socket_FD);
			}
			++iter_cur;
		}
	}

	/*
	Returns the socket_data associated with the socket.
	Precondition: Socket must exists in Socket_Data_Network (it must be owned by
		the network thread).
	*/
	boost::shared_ptr<socket_data> get_socket_data(const int socket_FD)
	{
		boost::mutex::scoped_lock lock(Socket_Data_mutex);
		std::map<int, boost::shared_ptr<socket_data> >::iterator
			iter = Socket_Data_Network.find(socket_FD);
		if(iter == Socket_Data_Network.end()){
			LOGGER << "violated precondition";
			exit(1);
		}
		return iter->second;
	}

	/*
	Removes socket_data from both Socket_Data_All and Socket_Data_Network.
	*/
	void remove_socket_data(const int socket_FD)
	{
		boost::mutex::scoped_lock lock(Socket_Data_mutex);
		Socket_Data_All.erase(socket_FD);
		Socket_Data_Network.erase(socket_FD);
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
	stop_networking:
		Before reactor is destroyed, and before workers are stopped. This should
		be called top stop the networking thread. This is stopped before the
		workers so we can finish all remaining jobs.
	*/
	virtual void connect(const std::string & host, const std::string & port,
		boost::shared_ptr<wrapper::info> & Info) = 0;
	virtual void finish_job(boost::shared_ptr<socket_data> & SD) = 0;
	virtual void start_networking() = 0;
	virtual void stop_networking() = 0;

private:
	/*
	Connected socket associated with socket_data. All access to these are locked
	with Socket_Data_mutex.
	Socket_Data_All: This is a collection of all socket_data objects. A
		socket_data object will be put in here upon add_socket_data() and removed
		upon remove_socket_data().
	Socket_Data_Network: This container stores socket_data which the network
		thread (thread in derived class) has access to. When a call back job is
		scheduled for a socket the coresponding socket_data will be removed from
		this container. This is a way of transferring ownership of a socket_data
		between threads.
	*/
	boost::mutex Socket_Data_mutex;
	std::map<int, boost::shared_ptr<socket_data> > Socket_Data_All;
	std::map<int, boost::shared_ptr<socket_data> > Socket_Data_Network;

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
	boost::function<void (const std::string & host, const std::string & port)> failed_connect_call_back;
	boost::function<void (socket_data::socket_data_visible & Socket)> connect_call_back;
	boost::function<void (socket_data::socket_data_visible & Socket)> disconnect_call_back;

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
	void run_call_back_job(boost::shared_ptr<socket_data> & SD)
	{
		if(SD->failed_connect_flag){
			failed_connect_call_back(SD->host, SD->port);
		}else{
			if(!SD->connect_flag){
				SD->connect_flag = true;
				connect_call_back(SD->Socket_Data_Visible);
			}
			assert(SD->Socket_Data_Visible.recv_call_back);
			assert(SD->Socket_Data_Visible.send_call_back);
			if(SD->recv_flag){
				SD->recv_flag = false;
				SD->Socket_Data_Visible.recv_call_back(SD->Socket_Data_Visible);
			}
			if(SD->send_flag){
				SD->send_flag = false;
				SD->Socket_Data_Visible.send_call_back(SD->Socket_Data_Visible);
			}
			if(SD->disconnect_flag){
				disconnect_call_back(SD->Socket_Data_Visible);
			}
		}

		//ignore dummy socket_data used to do failed_connect_call_back()
		if(SD->socket_FD != -1){
			transfer_socket_data_ownership(SD);
			finish_job(SD);
		}
	}

	void transfer_socket_data_ownership(boost::shared_ptr<socket_data> & SD)
	{
		boost::mutex::scoped_lock lock(Socket_Data_mutex);
		Socket_Data_Network.insert(std::make_pair(SD->socket_FD, SD));
	}
};
}
#endif

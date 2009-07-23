#ifndef H_NETWORK_REACTOR
#define H_NETWORK_REACTOR

//custom
#include "buffer.hpp"
#include "rate_limit.hpp"
#include "socket_data.hpp"
#include "wrapper.hpp"

//include
#include <boost/utility.hpp>
#include <logger.hpp>

//standard
#include <limits>

namespace network{

/*
The reactor is really a proactor because it has multiple threads which do call
backs. It was named reactor so the class which wraps it can be called proactor.
*/
class reactor
{
public:
	//derived class ctors must call this base class ctor
	reactor(
		boost::function<void (const std::string & host, const std::string & port,
			const ERROR error)> failed_connect_call_back_in,
		boost::function<void (socket_data & Socket)> connect_call_back_in,
		boost::function<void (socket_data & Socket)> disconnect_call_back_in
	):
		failed_connect_call_back(failed_connect_call_back_in),
		connect_call_back(connect_call_back_in),
		disconnect_call_back(disconnect_call_back_in),
		incoming_connections(0),
		max_incoming_connections(0),
		outgoing_connections(0),
		max_outgoing_connections(0)
	{
		wrapper::start_networking();

		int socket_IPv4 = wrapper::start_listener_IPv4("0");
		IPv4 = (socket_IPv4 != -1);
		close(socket_IPv4);

		int socket_IPv6 = wrapper::start_listener_IPv6("0");
		IPv6 = (socket_IPv6 != -1);
		close(socket_IPv6);

		if(!IPv4 && !IPv6){
			LOGGER << "error: neither IPv4 or IPv6 enabled";
			exit(1);
		}
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
		boost::mutex::scoped_lock lock(connect_job_mutex);
		connect_job.push_back(std::make_pair(host, port));
		connect_job_cond.notify_one();
	}

	//functions for getting/settings connection related things
	unsigned get_connections()
	{
		boost::mutex::scoped_lock lock(connections_mutex);
		return incoming_connections + outgoing_connections;
	}
	unsigned get_incoming_connections()
	{
		boost::mutex::scoped_lock lock(connections_mutex);
		return incoming_connections;
	}
	unsigned get_max_incoming_connections()
	{
		boost::mutex::scoped_lock lock(connections_mutex);
		return max_incoming_connections;
	}
	unsigned get_outgoing_connections()
	{
		boost::mutex::scoped_lock lock(connections_mutex);
		return outgoing_connections;
	}
	unsigned get_max_outgoing_connections()
	{
		boost::mutex::scoped_lock lock(connections_mutex);
		return max_outgoing_connections;
	}
	//sets maximum number of incoming/outgoing connections
	void set_max_connections(const unsigned max_incoming_connections_in,
		const unsigned max_outgoing_connections_in)
	{
		boost::mutex::scoped_lock lock(connections_mutex);
		max_incoming_connections = max_incoming_connections_in;
		max_outgoing_connections = max_outgoing_connections_in;
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

	//returns true if IPv4 networking available on this computer
	bool IPv4_enabled()
	{
		return IPv4;
	}
	//returns true if IPv6 networking available on this computer
	bool IPv6_enabled()
	{
		return IPv6;
	}

	//must be called after construction to start reactor threads
	void start()
	{
		for(int x=0; x<8; ++x){
			Workers.create_thread(boost::bind(&reactor::dispatch, this));
		}
		Workers.create_thread(boost::bind(&reactor::resolve, this));
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
	Called by the derived class to schedule a call back.
	Note: As a safety feature the shared_ptr passed to this function will be set
		to NULL so that an assert will be triggered if it's used. This stops
		someone from accidently introducing a thread-unsafe condition where they
		pass the socket data to this function, but then continue to use it.
	*/
	void call_back(boost::shared_ptr<socket_data> & SD)
	{
		boost::mutex::scoped_lock lock(call_back_job_mutex);
		call_back_job.push_back(SD);
		call_back_job_cond.notify_one();
		SD = boost::shared_ptr<socket_data>();
	}

	/*
	This is a special call back to be used when a connection fails before there is
	any socket or socket_data. This will need to be used when DNS resolution or
	socket allocation fails.
	*/
	void call_back_failed_connect(const std::string & host, const std::string & port,
		const ERROR Error)
	{
		boost::mutex::scoped_lock lock(call_back_job_mutex);

		/*
		Create a dummy socket_data so we can use the same mechanism for the
		failed_connect_call_back.
		*/
		boost::shared_ptr<socket_data> SD(new socket_data(-1, host, "", port));
		SD->failed_connect_flag = true;
		SD->error = Error;
		call_back_job.push_back(SD);
		call_back_job_cond.notify_one();
	}

	/*
	incoming_decrement:
		Decrements the incoming connection count.
		Note: Done by reactor::run_call_back_job().
		Precondition: incoming_connections != 0.
	incoming_increment:
		Increments the incoming connection count.
		Precondition: incoming_connections < max_incoming_connections.
	incoming_limit_reached:
		Return true if incoming connection limit reached.
	outgoing_decrement:
		Decrements the outgoing connection count.
		Note: Done by reactor::run_call_back_job().
		Precondition: outgoing_connections != 0.
	outgoing_increment:
		Increments the outgoing connection count.
		Precondition: outgoing_connections < max_outgoing_connections.
	outgoing_limit_reached:
		Return true if outgoing connection limit reached.
	*/
	void incoming_decrement()
	{
		boost::mutex::scoped_lock lock(connections_mutex);
		if(incoming_connections <= 0){
			LOGGER << "violated precondition";
			exit(1);
		}else{
			--incoming_connections;
		}
	}
	void incoming_increment()
	{
		boost::mutex::scoped_lock lock(connections_mutex);
		if(incoming_connections >= max_incoming_connections){
			LOGGER << "violated precondition";
			exit(1);
		}else{
			++incoming_connections;
		}
	}
	bool incoming_limit_reached()
	{
		boost::mutex::scoped_lock lock(connections_mutex);
		return incoming_connections >= max_incoming_connections;
	}
	void outgoing_decrement()
	{
		boost::mutex::scoped_lock lock(connections_mutex);
		if(outgoing_connections <= 0){
			LOGGER << "violated precondition";
			exit(1);
		}else{
			--outgoing_connections;
		}
	}
	void outgoing_increment()
	{
		boost::mutex::scoped_lock lock(connections_mutex);
		if(outgoing_connections >= max_outgoing_connections){
			LOGGER << "violated precondition";
			exit(1);
		}else{
			++outgoing_connections;
		}
	}
	bool outgoing_limit_reached()
	{
		boost::mutex::scoped_lock lock(connections_mutex);
		return outgoing_connections >= max_outgoing_connections;
	}

	/* Functions Reactors Must Define.
	connect:
		Establishes new connection. This is called by a worker after DNS
		resolution.
	finish_call_back:
		After worker does call backs this function will be called so that the
		socket is again monitored for activity.
	hard_connection_limit:
		This function returns the maximum possible number of sockets supported by
		the reactor. The number returned must always be the same during runtime.
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
	virtual void finish_call_back(boost::shared_ptr<socket_data> & SD) = 0;
	virtual void start_networking() = 0;
	virtual void stop_networking() = 0;

private:
	//true if protocol enabled
	bool IPv4;
	bool IPv6;

	/* Connection counts and limits.
	Note: All these must be locked with connections_mutex.
	max_connections:
		This is the maximum possible number of connections the reactor supports.
		The reactor_select() for example supports slightly less than 1024.
	incoming_connections:
		Number of connections remote hosts established with us.
	max_incoming_connections:
		Maximum possible incoming connections.
	outgoing_connections:
		Number of connections we established with remote hosts.
	max_outgoing_connection:
		Maximum possible outgoing connections.
	*/
	boost::mutex connections_mutex;
	unsigned incoming_connections;
	unsigned max_incoming_connections;
	unsigned outgoing_connections;
	unsigned max_outgoing_connections;

	//call backs
	boost::function<void (const std::string & host, const std::string & port,
		const network::ERROR error)> failed_connect_call_back;
	boost::function<void (socket_data & Socket)> connect_call_back;
	boost::function<void (socket_data & Socket)> disconnect_call_back;

	//worker threads to handle call backs and DNS resolution
	boost::thread_group Workers;

	/*
	Jobs to connect to a server.
	Note: connect_job_mutex locks all access to connect_job.
	Note: connect job contains std::pair<host, port>
	Note: connect_job_cond used for reactor::connect() function to signal the
		resove threads that a job is ready to be processed.
	*/
	boost::mutex connect_job_mutex;
	boost::condition_variable_any connect_job_cond;
	std::deque<std::pair<std::string, std::string> > connect_job;

	/*
	Jobs to do call backs.
	Note: call_back_job_mutex locks all access to call_back_job.
	Note: call_back_job_cond used for reactor_select to signal when call backs
		need to be done.
	*/
	boost::mutex call_back_job_mutex;
	boost::condition_variable_any call_back_job_cond;
	std::deque<boost::shared_ptr<socket_data> > call_back_job;

	void resolve()
	{
		while(true){
			std::pair<std::string, std::string> job;

			{//begin lock scope
			boost::mutex::scoped_lock lock(connect_job_mutex);
			while(connect_job.empty()){
				connect_job_cond.wait(connect_job_mutex);
			}
			job = connect_job.front();
			connect_job.pop_front();
			}//end lock scope

			//construction of wrapper::info does DNS lookup
			boost::shared_ptr<wrapper::info> Info(new wrapper::info(
				job.first.c_str(), job.second.c_str()));
			connect(job.first, job.second, Info);
		}
	}

	//threads which do call backs reside in this function
	void dispatch()
	{
		while(true){
			boost::shared_ptr<socket_data> job;

			{//begin lock scope
			boost::mutex::scoped_lock lock(call_back_job_mutex);
			while(call_back_job.empty()){
				call_back_job_cond.wait(call_back_job_mutex);
			}
			job = call_back_job.front();
			call_back_job.pop_front();
			}//end lock scope

			run_call_back_job(job);
		}
	}

	//do pending call backs
	void run_call_back_job(boost::shared_ptr<socket_data> & SD)
	{
		if(SD->failed_connect_flag){
			assert(SD->error != NONE);
			failed_connect_call_back(SD->host, SD->port, SD->error);
			close(SD->socket_FD);
		}else{
			if(!SD->connect_flag){
				SD->connect_flag = true;
				connect_call_back(*SD);
			}
			if(!SD->disconnect_flag && SD->recv_flag){
				assert(SD->recv_call_back);
				SD->recv_flag = false;
				SD->recv_call_back(*SD);
			}
			if(!SD->disconnect_flag && SD->send_flag){
				assert(SD->send_call_back);
				SD->send_flag = false;
				SD->send_call_back(*SD);
			}
			if(SD->disconnect_flag){
				disconnect_call_back(*SD);
				close(SD->socket_FD);
			}
		}

		if(SD->socket_FD != -1 && (SD->failed_connect_flag || SD->disconnect_flag)){
			if(SD->direction == INCOMING){
				incoming_decrement();
			}else{
				outgoing_decrement();
			}
		}

		/*
		Ignore dummy socket_data used to do failed_connect_call_back(). Also don't
		pass back sockets that were disconnected.
		*/
		if(SD->socket_FD != -1 && !SD->failed_connect_flag && !SD->disconnect_flag){
			finish_call_back(SD);
		}
	}
};
}
#endif

/*
The reactor should be interfaced with through this class. It is not possible to
directly interface with a specific reactor.
*/
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
	reactor():
		_incoming_connections(0),
		max_incoming_connections(0),
		_outgoing_connections(0),
		max_outgoing_connections(0)
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
		if(!S->info->resolved()){
			S->failed_connect_flag = true;
			S->sock_error = FAILED_RESOLVE;
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

	/*
	When finished with a socket return it here.
	Note: If a sock is not returned then the connection counts will get screwed
		up and the program will likely be terminated. For this reason it's
		inadviseable to use a reactor directly. Using the proactor makes it
		impossible to forget to return a sock.
	*/
	void put(boost::shared_ptr<sock> & S)
	{
		if(S->disconnect_flag || S->failed_connect_flag){
			if(S->socket_FD != -1){
				if(S->direction == INCOMING){
					incoming_decrement();
				}else{
					outgoing_decrement();
				}
			}
		}else{
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

	//returns current total connections, both incoming and outgoing
	unsigned connections()
	{
		boost::mutex::scoped_lock lock(connections_mutex);
		return _incoming_connections + _outgoing_connections;
	}

	//returns current incoming connections
	unsigned incoming_connections()
	{
		boost::mutex::scoped_lock lock(connections_mutex);
		return _incoming_connections;
	}

	//returns maximum allowed incoming connections
	unsigned get_max_incoming_connections()
	{
		boost::mutex::scoped_lock lock(connections_mutex);
		return max_incoming_connections;
	}

	//returns current outgoing connections
	unsigned outgoing_connections()
	{
		boost::mutex::scoped_lock lock(connections_mutex);
		return _outgoing_connections;
	}

	//returns maximum allowed outgoing connections
	unsigned get_max_outgoing_connections()
	{
		boost::mutex::scoped_lock lock(connections_mutex);
		return max_outgoing_connections;
	}

	/*
	Sets maximum number of incoming/outgoing connections.
	Note: max_incoming_connections_in + max_outgoing_connections_in must not
		exceed max_connection_supported().
	*/
	void set_max_connections(const unsigned max_incoming_connections_in,
		const unsigned max_outgoing_connections_in)
	{
		boost::mutex::scoped_lock lock(connections_mutex);
		if(max_incoming_connections_in + max_outgoing_connections_in
			> max_connections_supported())
		{
			LOGGER << "max_incoming + max_outgoing connections exceed implementation max";
			exit(1);
		}
		max_incoming_connections = max_incoming_connections_in;
		max_outgoing_connections = max_outgoing_connections_in;
	}

	//returns current download rate (bytes/second)
	unsigned current_download_rate()
	{
		return Rate_Limit.current_download_rate();
	}

	//returns current upload rate (bytes/second)
	unsigned current_upload_rate()
	{
		return Rate_Limit.current_upload_rate();
	}

	//returns maximum allowed download rate (bytes/second)
	unsigned get_max_download_rate()
	{
		return Rate_Limit.get_max_download_rate();
	}

	//returns maximum alloed upload rate (bytes/second)
	unsigned get_max_upload_rate()
	{
		return Rate_Limit.get_max_upload_rate();
	}

	//sets the maximum allowed download rate (bytes/second)
	void set_max_download_rate(const unsigned rate)
	{
		Rate_Limit.set_max_download_rate(rate);
	}

	//sets the maximum allowed upload rate (bytes/second)
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
			S->seen();
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
			S->seen();
			return S;
		}
	}

	/*
	Decrements the incoming connection count.
	Precondition: incoming_connections != 0.
	*/
	void incoming_decrement()
	{
		boost::mutex::scoped_lock lock(connections_mutex);
		if(_incoming_connections == 0){
			LOGGER << "violated precondition";
			exit(1);
		}else{
			--_incoming_connections;
		}
	}

	/*
	Increments the incoming connection count.
	Precondition: A sock must exist with a socket_FD != -1. This is because
		incoming_decrement() will be called for a sock with a socket_FD != -1. We
		want there to be a decrement for every increment.
	Precondition: incoming_connections < max_incoming_connections.
	*/
	void incoming_increment()
	{
		boost::mutex::scoped_lock lock(connections_mutex);
		if(_incoming_connections >= max_incoming_connections){
			LOGGER << "violated precondition";
			exit(1);
		}else{
			++_incoming_connections;
		}
	}

	//return true if incoming connection limit reached
	bool incoming_limit_reached()
	{
		boost::mutex::scoped_lock lock(connections_mutex);
		return _incoming_connections >= max_incoming_connections;
	}

	/*
	Decrements the outgoing connection count.
	Precondition: outgoing_connections != 0.
	*/
	void outgoing_decrement()
	{
		boost::mutex::scoped_lock lock(connections_mutex);
		if(_outgoing_connections == 0){
			LOGGER << "violated precondition";
			exit(1);
		}else{
			--_outgoing_connections;
		}
	}

	/*
	Increments the outgoing connection count.
	Precondition: A sock must exist with a socket_FD != -1. This is because
		outgoing_decrement() will be called for a sock with a socket_FD != -1. We
		want there to be a decrement for every increment.
	Precondition: outgoing_connections < max_outgoing_connections.
	*/
	void outgoing_increment()
	{
		boost::mutex::scoped_lock lock(connections_mutex);
		if(_outgoing_connections >= max_outgoing_connections){
			LOGGER << "violated precondition";
			exit(1);
		}else{
			++_outgoing_connections;
		}
	}

	//return true if outgoing connection limit reached
	bool outgoing_limit_reached()
	{
		boost::mutex::scoped_lock lock(connections_mutex);
		return _outgoing_connections >= max_outgoing_connections;
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
	unsigned _incoming_connections;
	unsigned max_incoming_connections;
	unsigned _outgoing_connections;
	unsigned max_outgoing_connections;
};
}//end namespace network
#endif

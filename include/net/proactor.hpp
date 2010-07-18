#ifndef H_NET_PROACTOR
#define H_NET_PROACTOR

//custom
#include "buffer.hpp"
#include "connection.hpp"
#include "connection_info.hpp"
#include "dispatcher.hpp"
#include "init.hpp"
#include "listener.hpp"
#include "nstream.hpp"
#include "rate_limit.hpp"
#include "select.hpp"

//include
#include <boost/shared_ptr.hpp>
#include <portable.hpp>

//standard
#include <map>
#include <queue>

namespace net{
class proactor : private boost::noncopyable
{
	net::init Init;
public:
	//if listener not specified then listener is not started
	proactor(
		const boost::function<void (connection_info &)> & connect_call_back,
		const boost::function<void (connection_info &)> & disconnect_call_back
	);

	//documentation in proactor.hpp

	//stop/start
	void start(boost::shared_ptr<listener> Listener_in);
	void stop();

	//connect/disconnect/send
	void connect(const endpoint & ep);
	void disconnect(const int connection_ID);
	void disconnect_on_empty(const int connection_ID);
	void send(const int connection_ID, buffer & send_buf);

	//info
	unsigned download_rate();
	unsigned upload_rate();

	//set options
	void set_connection_limit(const unsigned incoming_limit, const unsigned outgoing_limit);
	void set_max_download_rate(const unsigned rate);
	void set_max_upload_rate(const unsigned rate);

private:
	//lock for starting/stopping proactor
	boost::recursive_mutex start_stop_mutex;
	bool stopped;

	ID_manager ID_Manager;
	dispatcher Dispatcher;
	boost::shared_ptr<listener> Listener;
	rate_limit Rate_Limit;
	select Select;
	std::time_t network_loop_time;                        //used to run tasks once per second
	std::set<int> read_FDS, write_FDS;                    //sets to monitor with select
	std::map<int, boost::shared_ptr<connection> > Socket; //socket_FD associated with connection
	std::map<int, boost::shared_ptr<connection> > ID;     //connection_ID associated with connection
	unsigned incoming_connection_limit;                   //maximum allowed incoming connections
	unsigned outgoing_connection_limit;                   //maximum allowed outgoing connections
	unsigned incoming_connections;                        //current incoming connections
	unsigned outgoing_connections;                        //current outgoing connections

	/*
	Function call proxy. A thread adds a function object and network_thread makes
	the function call. This eliminates a lot of locking.
	*/
	boost::mutex relay_job_mutex;
	std::deque<boost::function<void ()> > relay_job;

	/*
	adjust_connection_limits:
		Called via relay_job proxy to adjust connection limits.
	add_socket:
		Add socket to be monitored.
		Note: P.first must be socket_FD.
	append_send_buf:
		Proxy-called using relay_job to append data to be sent to
		send_buf.
	check_timeouts:
		Disconnecte connections which have timed out.
	connect_relay:
		Start async connect to endpoint.
	disconnect:
		Disconnect a connection. If on_empty = true then connection disconnected
		when connection send_buf is empty.
	lookup_socket:
		Lookup connection by socket_FD. If connection not found then shared_ptr
		will be empty.
	lookup_ID:
		Lookup connection by connection_ID. If connection not found then
		shared_ptr will be empty.
	handle_async_connection:
		Called when a async connecting socket becomes writeable.
		Note: P.first must be socket_FD.
	network_loop:
		The network_thread resides in this function.
	process_relay_job:
		Called by network_thread to process relay jobs.
	remote_socket:
		Removes connection that corresponds to socket_FD.
	*/
	void adjust_connection_limits(const unsigned incoming_limit,
		const unsigned outgoing_limit);
	void add_socket(std::pair<int, boost::shared_ptr<connection> > P);
	void append_send_buf(const int connection_ID, boost::shared_ptr<buffer> B);
	void check_timeouts();
	void connect_relay(const endpoint ep);
	void disconnect(const int connection_ID, const bool on_empty);
	std::pair<int, boost::shared_ptr<connection> > lookup_socket(const int socket_FD);
	std::pair<int, boost::shared_ptr<connection> > lookup_ID(const int connection_ID);
	void handle_async_connection(std::pair<int, boost::shared_ptr<connection> > P);
	void network_loop();
	void process_relay_job();
	void remove_socket(const int socket_FD);

	thread_pool_global Thread_Pool;
};
}//end namespace net
#endif

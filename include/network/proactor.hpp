#ifndef H_NETWORK_PROACTOR
#define H_NETWORK_PROACTOR

//custom
#include "dispatcher.hpp"
#include "connection_info.hpp"
#include "listener.hpp"
#include "protocol.hpp"
#include "rate_limit.hpp"
#include "select.hpp"

//include
#include <atomic_int.hpp>
#include <boost/shared_ptr.hpp>
#include <thread_pool.hpp>

//standard
#include <map>

namespace network{
class proactor : private boost::noncopyable
{
	/*
	Timeout (seconds) before idle sockets disconnected. This has to be quite high
	otherwise client that download very slowly will be erroneously disconnected.
	*/
	static const unsigned idle_timeout = 600;

	class init
	{
	public:
		init();
		~init();
	} Init;

public:
	//if listener not specified then listener is not started
	proactor(
		const boost::function<void (connection_info &)> & connect_call_back,
		const boost::function<void (connection_info &)> & disconnect_call_back
	);

	/* Stop/Start
	start:
		Starts the proactor. Optionally specify a local endpoint to start a
		listener. Returns false if failed to start listener.
	stop:
		Stops the proactor.
		Note: It's not supported to start the proactor after it has been stopped.
	*/
	bool start();
	bool start(const endpoint & E);
	void stop();

	/* Connect/Disconnect/Send
	connect:
		Start async connection to host. Connect call back will happen if connects,
		or disconnect call back if connection fails.
	disconnect:
		Disconnect as soon as possible.
	disconnect_on_empty:
		Disconnect when send buffer becomes empty.
	send:
		Send data to specified connection if connection still exists.
	*/
	void connect(const std::string & host, const std::string & port);
	void disconnect(const int connection_ID);
	void disconnect_on_empty(const int connection_ID);
	void send(const int connection_ID, buffer & send_buf);


	/* Info
	download_rate:
		Returns download rate averaged over a few seconds (B/s).
	listen_port:
		Returns port listening on (empty if not listening).
	upload_rate:
		Returns upload rate averaged over a few seconds (B/s).
	*/
	unsigned download_rate();
	std::string listen_port();
	unsigned upload_rate();

	/* Get Options
	get_max_download_rate:
		Get maximum allowed download rate.
	get_max_upload_rate:
		Get maximum allowed upload rate.
	*/
	unsigned get_max_download_rate();
	unsigned get_max_upload_rate();

	/* Set Options
	set_connection_limit:
		Set connection limit for incoming and outgoing connections.
		Note: incoming_limit + outgoing_limit <= 1024.
	set_max_download_rate:
		Set maximum allowed download rate.
	set_max_upload_rate:
		Set maximum allowed upload rate.
	*/
	void set_connection_limit(const unsigned incoming_limit, const unsigned outgoing_limit);
	void set_max_download_rate(const unsigned rate);
	void set_max_upload_rate(const unsigned rate);

private:

	//lock for starting/stopping proactor
	boost::recursive_mutex start_stop_mutex;

	/*
	This class allocates connection_IDs. There are a few problems this class
	addresses:
	1. We don't want to let the socket_FDs leave the proactor because the
		proactor operates on sockets asynchronously and may disconnect/reconnect
		while a call back is taking place.
	2. The connection_IDs would have the same problem as #1 if we reused
		connection_IDs. To avoid the problem we continually allocate
		connection_IDs one greater than the last allocated connection_ID.
	3. It is possible that the connection_IDs will wrap around and reuse existing
		connection_IDs. We maintain a set of used connection_IDs so we can prevent
		this.
	*/
	class ID_manager : private boost::noncopyable
	{
	public:
		ID_manager();

		/*
		allocate:
			Allocate connection_ID. The deallocate function should be called when
			done with the connection_ID.
		deallocate:
			Deallocate connection_ID. Allows the connection_ID to be reused.
		*/
		int allocate();
		void deallocate(int connection_ID);

	private:
		boost::mutex Mutex;      //locks all access to object
		int highest_allocated;   //last connection_ID allocated
		std::set<int> allocated; //set of all connection_IDs allocated
	};

	class connection : private boost::noncopyable
	{
	private:
		bool connected;          //false if async connect in progress
		ID_manager & ID_Manager; //allocates and deallocates connection_ID
		std::set<endpoint> E;    //endpoints to try to connect to
		std::time_t last_seen;   //last time there was activity on socket
		int socket_FD;           //file descriptor, will change after open_async()

	public:
		/*
		Ctor for outgoing connection.
		Note: This ctor does DNS resolution if needed.
		*/
		connection(
			ID_manager & ID_Manager_in,
			const std::string & host_in,
			const std::string & port_in
		);

		//ctor for incoming connection
		connection(
			ID_manager & ID_Manager_in,
			const boost::shared_ptr<nstream> & N_in
		);

		~connection();

		//modified by proactor
		bool disc_on_empty; //if true we disconnect when send_buf is empty
		buffer send_buf;    //bytes to send appended to this

		//const
		const boost::shared_ptr<nstream> N; //connection object
		const std::string host;             //hostname or IP to connect to
		const std::string port;             //port number
		const int connection_ID;            //see above

		/* WARNING
		This should not be modified by any thread other than a dispatcher
		thread. Modifying this from the network_thread is not thread safe unless
		it hasn't yet been passed to dispatcher.
		*/
		boost::shared_ptr<connection_info> CI;

		/*
		half_open:
			Returns true if async connect in progress.
		is_open:
			Returns true if async connecte succeeded. This is called after half
			open socket becomes writeable.
			Postcondition: If true returned then half_open() will return false.
		open_async:
			Open async connection, returns false if couldn't allocate socket.
			Postcondition: socket_FD, and CI will change.
		socket:
			Returns file descriptor.
			Note: Will return different file descriptor after open_async().
		timed_out:
			Returns true if connection timed out.
		touch:
			Resets timer used for timeout.
		*/
		bool half_open();
		bool is_open();
		bool open_async();
		int socket();
		bool timed_out();
		void touch();
	};

	boost::thread network_thread;
	ID_manager ID_Manager;
	dispatcher Dispatcher;
	listener Listener;
	rate_limit Rate_Limit;
	thread_pool Thread_Pool;
	select Select;
	std::set<int> read_FDS, write_FDS;                    //sets to monitor with select
	std::map<int, boost::shared_ptr<connection> > Socket; //socket_FD associated with connection
	std::map<int, boost::shared_ptr<connection> > ID;     //connection_ID associated with connection
	unsigned incoming_connection_limit;                   //maximum allowed incoming connections
	unsigned outgoing_connection_limit;                   //maximum allowed outgoing connections
	unsigned incoming_connections;                        //current incoming connections
	unsigned outgoing_connections;                        //current outgoing connections

	//function proxy, function calls for network_thread to make
	boost::mutex network_thread_call_mutex;
	std::deque<boost::function<void ()> > network_thread_call;

	/*
	adjust_connection_limits:
		Called via network_thread_call proxy to adjust connection limits.
	add_socket:
		Add socket to be monitored.
		Note: P.first must be socket_FD.
	append_send_buf:
		Proxy-called using network_thread_call to append data to be sent to
		send_buf.
	check_timeouts:
		Disconnecte connections which have timed out.
	disconnect:
		Proxy-called using network_thread_call to disconnect a connection.
		Note: If on_empty = true then connection disconnected when connection
			send_buf is empty.
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
	process_network_thread_call:
		The network loop calls this to do function calls scheduled by adding an
		element to network_thread_call.
	remote_socket:
		Removes connection that corresponds to socket_FD.
	*/
	void adjust_connection_limits(const unsigned incoming_limit,
		const unsigned outgoing_limit);
	void add_socket(std::pair<int, boost::shared_ptr<connection> > P);
	void append_send_buf(const int connection_ID, boost::shared_ptr<buffer> B);
	void check_timeouts();
	void disconnect(const int connection_ID, const bool on_empty);
	std::pair<int, boost::shared_ptr<connection> > lookup_socket(const int socket_FD);
	std::pair<int, boost::shared_ptr<connection> > lookup_ID(const int connection_ID);
	void handle_async_connection(std::pair<int, boost::shared_ptr<connection> > P);
	void network_loop();
	void process_network_thread_call();
	void remove_socket(const int socket_FD);

	/*
	When connect() is called a call to resolve relay is done with the
	network_thread_call proxy. This is done so the network thread can evaluate
	connection limits before starting the connection. If the connection can be
	made then a job is scheduled with the thread pool to do resolution.
	Note: Thread pool threads run in the resolve() function. Be careful about
		shared state.
	*/
	void resolve_relay(const std::string host, const std::string port);
	void resolve(const std::string & host, const std::string & port);
};
}//end namespace network
#endif

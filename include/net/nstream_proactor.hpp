#ifndef H_NET_NSTREAM_PROACTOR
#define H_NET_NSTREAM_PROACTOR

//custom
#include "buffer.hpp"
#include "init.hpp"
#include "listener.hpp"
#include "nstream.hpp"
#include "rate_limit.hpp"
#include "select.hpp"

//include
#include <boost/shared_ptr.hpp>
#include <channel.hpp>
#include <portable.hpp>
#include <thread_pool.hpp>

//standard
#include <map>
#include <queue>
#include <string>

namespace net{
class nstream_proactor : private boost::noncopyable
{
	static const unsigned connect_timeout = 60;
	static const unsigned idle_timeout = 120;
	net::init Init;
public:

	//direction
	enum dir_t{
		incoming_dir, //remote host initiated connection
		outgoing_dir  //we initiated connection
	};

	//errors for disconnect call back
	enum error_t{
		no_error,               //used when we close connection
		connect_error,          //failed to connect to remote host
		connection_reset_error, //remote end closed connection
		timeout_error,          //connection timed out
		listen_error,           //failed to start listener
		limit_error             //conn closed because it would exceed conn limit
	};

	//transport type (what type of connection)
	enum tran_t{
		nstream_tran,        //TCP (nstream)
		nstream_listen_tran, //listener for incoming TCP (nstream)
		ndgram_tran          //UDP (ndgram)
	};

	//information to describe the connection, included with each event
	class conn_info
	{
	public:
		conn_info(
			const boost::uint64_t conn_ID_in,
			const dir_t dir_in,
			const tran_t tran_in,
			const boost::optional<endpoint> & local_ep_in = boost::optional<endpoint>(),
			const boost::optional<endpoint> & remote_ep_in = boost::optional<endpoint>()
		);
		boost::uint64_t conn_ID;
		dir_t dir;
		tran_t tran;
		boost::optional<endpoint> local_ep;
		boost::optional<endpoint> remote_ep;
	};

	class connect_event
	{
	public:
		connect_event(const boost::shared_ptr<const conn_info> & info_in);
		boost::shared_ptr<const conn_info> info;
	};

	class disconnect_event
	{
	public:
		disconnect_event(
			const boost::shared_ptr<const conn_info> & info_in,
			const error_t error_in
		);
		boost::shared_ptr<const conn_info> info;
		error_t error;
	};

	class recv_event
	{
	public:
		recv_event(
			const boost::shared_ptr<const conn_info> & info_in,
			const buffer & buf_in
		);
		boost::shared_ptr<const conn_info> info;
		buffer buf;
	};

	class send_event
	{
	public:
		send_event(
			const boost::shared_ptr<const conn_info> & info_in,
			const unsigned last_send_in,
			const unsigned send_buf_size_in
		);
		boost::shared_ptr<const conn_info> info;
		unsigned last_send;     //bytes sent in last send
		unsigned send_buf_size; //size of send_buf
	};

	nstream_proactor(
		const boost::function<void (connect_event)> & connect_call_back_in,
		const boost::function<void (disconnect_event)> & disconnect_call_back_in,
		const boost::function<void (recv_event)> & recv_call_back_in,
		const boost::function<void (send_event)> & send_call_back_in
	);

	/* All of these functions are asynchronous.
	connect:
		Connect to specified endpoint.
	disconnect:
		Disconnect a connection.
	listen:
		Start listener on local endpoint. Returns port listening on or empty
		string if cannot start listener. If cannot start listener disconnect call
		back will be done.
	send:
		Send buf to specified connection. If close_on_empty is true the
		connection will be closed when the send_buf becomes empty.
	*/
	void connect(const endpoint & ep);
	void disconnect(const boost::uint64_t conn_ID);
	channel::future<std::string> listen(const endpoint & ep);
	void send(const boost::uint64_t conn_ID, const buffer & buf,
		const bool close_on_empty = false);

private:
	/* Call Back Dispatcher
	Does multiple concurrent call backs but does not do concurrent call backs on
	the same connection. Will block the proactor internal thread if there is a
	backlog of jobs. This stops unbounded memory usage on systems with slow disk
	I/O that can't keep up with network I/O.
	*/
	class dispatcher
	{
		static const unsigned threads = 4;
		static const unsigned max_buf = 8192;
	public:
		dispatcher(
			const boost::function<void (connect_event)> & connect_call_back_in,
			const boost::function<void (disconnect_event)> & disconnect_call_back_in,
			const boost::function<void (recv_event)> & recv_call_back_in,
			const boost::function<void (send_event)> & send_call_back_in
		);
		~dispatcher();

		/*
		connect:
			Dispatch connect event.
		disconnect:
			Dispatch disconnect event.
		join:
			Block until no dispatch jobs.
		recv:
			Dispatch recv event.
		send:
			Dispatch send event.
		*/
		void connect(const connect_event & CE);
		void disconnect(const disconnect_event & DE);
		void join();
		void recv(const recv_event & RE);
		void send(const send_event & SE);

	private:
		const boost::function<void (connect_event)> connect_call_back;
		const boost::function<void (disconnect_event)> disconnect_call_back;
		const boost::function<void (recv_event)> recv_call_back;
		const boost::function<void (send_event)> send_call_back;

		boost::mutex mutex;                          //lock for all data members
		boost::thread_group workers;                 //threads in dispatch() function
		boost::condition_variable_any consumer_cond; //notify_one when job taken from queue
		boost::condition_variable_any producer_cond; //notify_one when job added to queue
		boost::condition_variable_any empty_cond;    //notify_all when no jobs scheduled or running
		std::set<boost::uint64_t> memoize;           //memoize for conn_IDs
		boost::uint64_t producer_cnt;                //number of jobs produced
		unsigned job_cnt;                            //queued + running jobs
		std::list<std::pair<boost::uint64_t, boost::function<void()> > > Job;

		/*
		dispatch:
			Dispatcher threads wait in this function for jobs.
		*/
		void dispatch();
	};

	/*
	Base class for different types of connections. This allows us to easily add
	new types of connections to the proactor.
	*/
	class conn : private boost::noncopyable
	{
	public:
		virtual ~conn();
		/*
		ID:
			Returns connection ID.
		info:
			Return conn_info.
		read:
			Perform read operation.
			Precondition: select must say socket needs read.
		set_error:
			Set error to be passed to disconnect call back.
		socket:
			Returns socket file descriptor.
		timed_out:
			Called once per second. If returns true the connection is removed.
		write:
			Perform write operation.
			Precondition: select must say socket can write.
		*/
		virtual boost::shared_ptr<const conn_info> info() = 0;
		virtual void read() = 0;
		virtual void schedule_send(const buffer & buf, const bool close_on_empty);
		virtual void set_error(const error_t error_in) = 0;
		virtual int socket() = 0;
		virtual bool timed_out();
		virtual void write();
	};

	/*
	Holds all connections. Contains functions to add/remove connections. The
	connections tell this container when they're ready to read or write. This
	container allocates conn_IDs.
	*/
	class conn_container
	{
	public:
		conn_container();
		/*
		add:
			Add connection.
		check_timeouts:
			Check timeouts on all connections. Remove timed out.
		new_conn_ID:
			Returns new unique conn_ID.
		read_set:
			Returns socket set that needs to be monitored for read readyness.
		monitor_read:
			Add socket to set of sockets to monitor for read readyness.
		monitor_write:
			Add socket to set of sockets to monitor for write readyness.
		perform_reads:
			Perform read on all sockets in set.
		perform_write:
			Perform write on all sockets in set.
		remove:
			Remove connection that corresponds to conn_ID.
		unmonitor_read:
			Remove socket from set of sockets to monitor for read readyness.
		unmonitor_write:
			Remove socket from set of sockets to monitor for write readyness.
		write_set:
			Returns socket set that needs tob e monitored for write readyness.
		*/
		void add(const boost::shared_ptr<conn> & C);
		void check_timeouts();
		boost::uint64_t new_conn_ID();
		std::set<int> read_set();
		void monitor_read(const int socket_FD);
		void monitor_write(const int socket_FD);
		void perform_reads(const std::set<int> & read_set_in);
		void perform_writes(const std::set<int> & write_set_in);
		void remove(const boost::uint64_t conn_ID);
		void schedule_send(const boost::uint64_t conn_ID, const buffer & buf,
			const bool close_on_empty);
		void unmonitor_read(const int socket_FD);
		void unmonitor_write(const int socket_FD);
		std::set<int> write_set();

	private:
		boost::uint64_t unused_conn_ID;
		std::time_t last_time;
		std::set<int> _read_set;
		std::set<int> _write_set;
		std::map<boost::uint64_t, boost::shared_ptr<conn> > ID;
		std::map<int, boost::shared_ptr<conn> > Socket;
		unsigned incoming_conn_limit;
		unsigned outgoing_conn_limit;
		unsigned incoming_conns;
		unsigned outgoing_conns;
	};

	//wraps nstream
	class conn_nstream : public conn
	{
	public:
		//ctor for outgoing connection
		conn_nstream(
			dispatcher & Dispatcher_in,
			conn_container & Conn_Container_in,
			const endpoint & ep
		);
		//ctor for incoming connection (used by conn_listener)
		conn_nstream(
			dispatcher & Dispatcher_in,
			conn_container & Conn_Container_in,
			boost::shared_ptr<nstream> N_in
		);
		virtual ~conn_nstream();
		virtual boost::shared_ptr<const conn_info> info();
		virtual void read();
		virtual void schedule_send(const buffer & buf, const bool close_on_empty_in);
		virtual void set_error(const error_t error_in);
		virtual int socket();

//rename timed_out to tick(), have it return void
		virtual bool timed_out();
		virtual void write();
	private:
		dispatcher & Dispatcher;

//DEBUG, bad design, conns should not have access to container they're in
		conn_container & Conn_Container;

//refs to sets in conn_container
		//std::set<int> & read_set;
		//std::set<int> & write_set;

//this can be used to schedule remove
		//std::set<boost::uint64_t> & remove

		boost::shared_ptr<nstream> N;
		int socket_FD;                   //keep copy so we know this after nstream close
		buffer send_buf;                 //stores bytes that need to be sent
		bool close_on_empty;             //when true close when send_buf becomes empty
		bool half_open;                  //if true async connect in progress
		std::time_t timeout;             //time at which this conn times out
		error_t error;                   //holds error for disconnect
		boost::shared_ptr<const conn_info> _info; //info passed to call backs
		/*
		touch:
			Updates timeout timer.
		*/
		void touch();
	};

	//wraps listener for nstreams, adds conn_nstreams to conn_container
	class conn_listener : public conn
	{
	public:
		conn_listener(
			dispatcher & Dispatcher_in,
			conn_container & Conn_Container_in,
			const endpoint & ep
		);
		virtual ~conn_listener();
		virtual boost::shared_ptr<const conn_info> info();
		virtual void read();
		virtual void set_error(const error_t error_in);
		virtual int socket();
		/*
		ep:
			Returns endpoint for listener or nothing if not listening.
		*/
		boost::optional<net::endpoint> ep();
	private:
		dispatcher & Dispatcher;
		conn_container & Conn_Container;
		listener Listener;
		int socket_FD;                            //keep copy so we know this after listener close
		error_t error;                            //holds error for disconnect
		boost::shared_ptr<const conn_info> _info; //info passed to call backs
	};

	//relay functions called by Internal_TP
	void connect_relay(const endpoint & ep);
	void disconnect_relay(const boost::uint64_t conn_ID);
	void listen_relay(const endpoint ep, channel::promise<std::string> promise);
	void send_relay(const boost::uint64_t conn_ID, const buffer buf,
		const bool close_on_empty);

	/*
	main_loop:
		Main network processing loop.
	*/
	void main_loop();

	select Select;
	dispatcher Dispatcher;
	conn_container Conn_Container;
	thread_pool Internal_TP;
};
}//end namespace net
#endif

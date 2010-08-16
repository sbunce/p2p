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
#include <portable.hpp>
#include <thread_pool.hpp>

//standard
#include <map>
#include <queue>

namespace net{
//DEBUG, rename to proactor when ready to replace current proactor
class nstream_proactor : private boost::noncopyable
{
	static const unsigned connect_timeout = 60;
	static const unsigned idle_timeout = 600;
	net::init Init;
public:

	enum dir_t{
		incoming, //remote host initiated connection
		outgoing  //we initiated connection
	};

	struct connect_event{
		boost::uint64_t conn_ID;
		dir_t dir;
		endpoint ep;
	};
	struct disconnect_event{
		boost::uint64_t conn_ID;
	};
	struct recv_event{
		boost::uint64_t conn_ID;
		buffer buf;
	};
	struct send_event{
		boost::uint64_t conn_ID;
		unsigned send_buf_size;	
	};

	nstream_proactor(
		const boost::function<void (connect_event)> & connect_call_back_in,
		const boost::function<void (disconnect_event)> & disconnect_call_back_in,
		const boost::function<void (recv_event)> & recv_call_back_in,
		const boost::function<void (send_event)> & send_call_back_in
	);

	/*
	listen:
		Start listener on local endpoint.
	*/
	void listen(const endpoint & ep);

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
		static const unsigned max_buf = 1024;
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
		unsigned job_cnt;                            //queued + running jobs
		std::list<std::pair<boost::uint64_t, boost::function<void()> > > Job;

		/*
		dispatch:
			Dispatcher threads wait in this function for jobs.
		*/
		void dispatch();
	};

	class conn : private boost::noncopyable
	{
	public:
		virtual ~conn(){}
		/*
		ID:
			Returns connection ID.
		read:
			Perform read operation.
			Precondition: select must say socket needs read.
		socket:
			Returns socket file descriptor.
		timed_out:
			Called once per second. If returns true the connection is removed.
		write:
			Perform write operation.
			Precondition: select must say socket can write.
		*/
		virtual boost::uint64_t ID() = 0;
		virtual void read() = 0;
		virtual int socket() = 0;
		virtual bool timed_out(){ return false; }
		virtual void write(){}
	};

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
		void perform_reads(std::set<int> read_set_in);
		void perform_writes(std::set<int> write_set_in);
		void remove(const boost::uint64_t conn_ID);
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
	};

	class conn_nstream : public conn
	{
	public:
		//ctor for incoming connection
		conn_nstream(
			dispatcher & Dispatcher_in,
			conn_container & Conn_Container_in,
			boost::shared_ptr<nstream> N_in
		);

		//documentation in base class
		virtual boost::uint64_t ID();
		virtual void read();
		virtual int socket();
		virtual bool timed_out();
		virtual void write();

	private:
		dispatcher & Dispatcher;
		conn_container & Conn_Container;
		boost::uint64_t conn_ID;
		int socket_FD;
		boost::shared_ptr<nstream> N;
	};

	class conn_listener : public conn
	{
	public:
		conn_listener(
			dispatcher & Dispatcher_in,
			conn_container & Conn_Container_in,
			const endpoint & ep
		);

		//documentation in base class
		virtual boost::uint64_t ID();
		virtual void read();
		virtual int socket();

	private:
		dispatcher & Dispatcher;
		conn_container & Conn_Container;
		boost::uint64_t conn_ID;
		int socket_FD;
		listener Listener;
	};

	select Select;
	conn_container Conn_Container;

	/*
	listen_relay:
		Start listener.
	main_loop:
		Main network processing loop.
	*/
	void listen_relay(const endpoint ep);
	void main_loop();

	//does call backs, destroyed after Internal_TP to insure we don't lose jobs
	dispatcher Dispatcher;

	/*
	This thread_pool contains 1 thread. This thread can access any data member
	of the proactor. The main_loop continually schedules itself with this TP.
	Also, public functions can schedule the calling of private functions with
	this TP to be thread safe.
	*/
	thread_pool Internal_TP;
};
}//end namespace net
#endif

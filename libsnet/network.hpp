//THREADSAFE
#ifndef H_NETWORKING
#define H_NETWORKING

//boost
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>

//custom
#include "rate_limit.hpp"

//include
#include <atomic_int.hpp>
#include <buffer.hpp>
#include <logger.hpp>
#include <speed_calculator.hpp>

//networking
#ifdef WIN32
	#define MSG_NOSIGNAL 0  //disable SIGPIPE on send()
	#define FD_SETSIZE 1024
	#include <winsock.h>
#else
	#include <arpa/inet.h>
	#include <fcntl.h>
	#include <netinet/in.h>
	#include <sys/socket.h>
#endif

//std
#include <cstdlib>
#include <limits>
#include <map>
#include <queue>
#include <vector>

class network : private boost::noncopyable
{
public:
	enum direction{
		OUTGOING,
		INCOMING
	};

	/* ctor parameters
	boost::bind is the nicest way to do a callback to the member function of a
	specific object. ex:
		boost::bind(&http::connect_call_back, &HTTP, _1, _2, _3, _4)

	connect_call_back:
		Called when server connected to. The direction indicates whether we
		established connection (direction OUTGOING), or if someone established a
		connection to us (direction INCOMING). The recv_buff and send_buff are
		passed to this because the user might want to reserve space in the buffers
		or add something to send_buff.
	recv_call_back:
		Called after data received (received data in recv_buff). A return value of 
		true will keep the server connected, but false will cause it to be
		disconnected.
	send_call_back:
		Called after data sent. The send_buff will contain data that hasn't yet
		been sent. A return value of true will keep the server connected, but
		false will cause it to be disconnected.
	disconnect_call-back:
		Called when a connected socket disconnects.
	connect_time_out:
		Seconds before connection attempt times out.
	socket_time_out:
		Seconds before socket with no I/O times out.
	listen_port:
		Port to listen on for incoming connections. The default (if no parameter
		specified) is to not listen for incoming connections.
	*/
	network(
		boost::function<void (int socket_FD, buffer & recv_buff, buffer & send_buff, direction Direction)> connect_call_back_in,
		boost::function<bool (int socket_FD, buffer & recv_buff, buffer & send_buff)> recv_call_back_in,
		boost::function<bool (int socket_FD, buffer & recv_buff, buffer & send_buff)> send_call_back_in,
		boost::function<void (int socket_FD)> disconnect_call_back_in,
		const int connect_time_out_in,
		const int socket_time_out_in,
		int listen_port = -1
	);
	~network();

	/*
	add_connection:
		Establish connection to specified IP and port. Returns true if connection
		will be tried, or false if outgoing_connection limit reached.
	current_download_rate:
		Returns current download_rate (bytes per second).
	current_upload_rate:
		Returns current upload_rate (bytes per second).
	*/
	bool add_connection(const std::string & IP, const int port);
	unsigned connections();
	unsigned current_download_rate();
	unsigned current_upload_rate();
	unsigned get_max_download_rate();
	unsigned get_max_upload_rate();
	void set_max_download_rate(const unsigned rate);
	void set_max_upload_rate(const unsigned rate);

private:
	//thread for main_loop
	boost::thread select_loop_thread;

	fd_set master_FDS;  //master file descriptor set
	fd_set read_FDS;    //what sockets can read non-blocking
	fd_set write_FDS;   //what sockets can write non-blocking
	fd_set connect_FDS; //sockets in progress of connecting

	int begin_FD; //first socket
	int end_FD;   //one past last socket
	int listener; //socket of listener (-1 means no listener)

	/*
	This is used for the self-pipe trick. A pipe is opened and put in the fd_set
	select() will monitor. We can asynchronously signal select() to return by
	writing a byte to the read FD of the pipe.
	*/
	int selfpipe_read;        //put in master_FDS
	int selfpipe_write;       //written to get select to return
	char selfpipe_discard[8]; //used to discard selfpipe bytes

	//call backs
	boost::function<void (int, buffer &, buffer &, direction)> connect_call_back;
	boost::function<bool (int, buffer &, buffer &)> recv_call_back;
	boost::function<bool (int, buffer &, buffer &)> send_call_back;
	boost::function<void (int)> disconnect_call_back;

	/*
	When no writes are pending CPU time is saved by not checking sockets for
	writeability. When this is zero there are no writes that need to be done.
	Ex: 5 would mean there are 5 send_buff with data in them.
	*/
	int writes_pending;

	/*
	The number of outgoing and incoming connections. Outgoing includes
	connections that are in progress. The connection limit applies to both
	incoming and outgoing (incoming/outgoing may not exceed connection_limit).
	*/
	const int connection_limit;
	atomic_int<int> outgoing_connections;
	atomic_int<int> incoming_connections;

	/*
	connect_time_out - how many seconds until connection attempt cancelled
	socket_time_out  - how many seconds of no I/O before socket disconnected
	*/
	const int connect_time_out;
	const int socket_time_out;

	//data needed for each socket
	class socket_data
	{
	public:
		/*
		The incoming_or_outgoing parameter is either the outgoing_connections or
		incoming_connections variable. It is decremented when the socket_data is
		destroyed.
		*/
		socket_data(
			int & writes_pending_in,
			atomic_int<int> & incoming_or_outgoing_in
		):
			writes_pending(writes_pending_in),
			incoming_or_outgoing(incoming_or_outgoing_in),
			last_seen(std::time(NULL))
		{}
		~socket_data()
		{
			if(!send_buff.empty()){
				--writes_pending;
			}
			--incoming_or_outgoing;
		}
		int & writes_pending;
		atomic_int<int> & incoming_or_outgoing;
		std::time_t last_seen;
		buffer recv_buff;
		buffer send_buff;
	};

	//connected socket associated with socket_data
	std::map<int, boost::shared_ptr<socket_data> > Socket_Data;

	/*
	Connecting socket associated with socket_data. Elements of this will be
	copied to the Socket_Data container when the socket connects.
	*/
	std::map<int, boost::shared_ptr<socket_data> > Connecting_Socket_Data;

	/*
	The add_connection function stores servers to connect to here. This handoff
	is done because it requires less locking.
	std::pair<IP, port>
	*/
	boost::recursive_mutex connection_queue_mutex;
	std::queue<std::pair<std::string, int> > connection_queue;

	//keeps track of total upload/download rates and to do rate limiting
	rate_limit Rate_Limit;

	/*
	add_socket:
		Add socket to master_FDS, adjusts begin_FD and end_FD.
	check_timeouts:
		Disconnect sockets that have timed out.
	create_socket_data:
		This function will do one of the following.
		1. If socket has socket_data in Connecting_Socket_Data it will be moved to
		   Socket_Data.
		2. If socket has no socket_data in Connecting_Socket_Data then new
		   socket_data will be created.
		3. If socket_data exists in Socket_Data, a pointer to it will be returned.
		Note: Returned socket data pointer will only be good so long as shared_ptr
		      to socket_data exists in Socket_Data.
		Note: This function does the connect_call_back.
	establish_incoming_connections:
		Handle incoming connection on listener.
	establish_outgoing_connection:
		Called by select_loop to add connections queued by add_connection().
	remove_socket:
		Remove socket from all fd_set's, adjusts min_FD and end_FD.
	start_listener:
		Accept incoming connections on specified port. Called by ctor if listening
		port is specified as last ctor parameter.
	*/
	void add_socket(const int socket_FD);
	void check_timeouts();
	socket_data * create_socket_data(const int socket_FD);
	void establish_incoming_connections();
	void establish_outgoing_connection(const std::string & IP, const int port);
	void process_connection_queue();
	void remove_socket(const int socket_FD);
	void select_loop();
	void start_listener(const int port);
	void start_selfpipe();
};
#endif

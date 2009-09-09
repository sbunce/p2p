/*
This reactor is the most portable. It uses the select() call which is the most
portable way of doing I/O multiplexing. The select() call is synchronous but we
make it somewhat asynchronous by handing off jobs to another thread (the threads
in the proactor).
*/
//THREADSAFE, THREAD-SPAWNING
#ifndef H_NETWORK_REACTOR_SELECT
#define H_NETWORK_REACTOR_SELECT

//custom
#include "reactor.hpp"

//standard
#include <set>

namespace network{
class reactor_select : public reactor
{
public:
	reactor_select(
		const bool duplicates_allowed_in,
		const std::string & port = ""
	);
	~reactor_select();

	//documentation in reactor class
	virtual unsigned connections_supported();
	virtual void start();
	virtual void stop();
	virtual void trigger_selfpipe();

private:
	//thread for main_loop
	boost::thread main_loop_thread;

	//if this is true then duplicate connections are allowed
	const bool duplicates_allowed;

	fd_set master_read_FDS;  //sockets to check for recv() readyness
	fd_set master_write_FDS; //sockets to check for send() readyness
	fd_set read_FDS;         //used by select()
	fd_set write_FDS;        //used by select()

	/*
	Sockets in progress of connecting. When no bytes can be sent (because of rate
	limit) write_FDS is set equal to this so only connecting sockets are
	monitored (to see when they connect).
	*/
	fd_set connect_FDS;

	//select() requires these
	int begin_FD; //first socket
	int end_FD;   //one past last socket

	/*
	Listening sockets. If on a dualstack OS then listener_IPv4 = listener_IPv6.
	If -1 then there is no listener.
	*/
	int listener_IPv4;
	int listener_IPv6;

	/*
	This is used for the selfpipe trick. A pipe is opened and put in the fd_set
	select() will monitor. We can asynchronously signal select() to return by
	writing a byte to the read FD of the pipe.
	*/
	int selfpipe_read;  //put in master_FDS
	int selfpipe_write; //written to get select to return

	/*
	The sock's in this container are monitored by select for activity. When
	something is done to the sock (which sets a flag) the sock is removed from
	this container and put in the job container so a client can deal with the
	sock.
	*/
	std::map<int, boost::shared_ptr<sock> > Monitored;

	/*
	add_socket (version which takes socket file descriptor):
		Add socket to be monitored. This function is used when there is no
		sock to be associated with the socket (this is the case with listeners
		and the self-pipe read).
	add_socket (version which takes network::sock):
		Add socket to be monitored. This function is used when there is sock
		to be associated with the socket.
	check_timeouts:
		Checks all monitored sockets to see if they've timed out.
	establish_incoming:
		Estabishes incoming connections on the specified listener.
	get_monitored:
		Retrieves sock from Monitored container.
		Precondition: Element in Monitored with socket_FD as key must exist.
	main_loop:
		This is the main loop of the reactor from which all internal reactor
		threads are called.
	main_loop_recv:
		When a socket needs to recv() this function is called.
	main_loop_send:
		When a socket needs to send() this function is called.
	process_connect_job:
		Gets connection jobs from reactor::get_job_connect() and starts
		asynchronous connection.
	process_finished_job:
		Gets finished jobs from reactor::get_finished_job() and starts monitoring
		the sockets for activity.
	process_force:
		If there are any socks which need to have a call back forced this function
		will do it.
	remove_socket (version which takes socket file descriptor):
		Stops socket from being monitored. Does not remove the Monitored element
		associated with the socket_FD.
	remove_socket (version which takes network::sock):
		Stops socket from being monitored. Removes the Monitored element
		associated with the socket_FD.
		Precondition: sock must exist in Monitored.
	*/
	void add_socket(const int socket_FD);
	void add_socket(boost::shared_ptr<sock> & S);
	void check_timeouts();
	void establish_incoming(const int listener);
	boost::shared_ptr<sock> get_monitored(const int socket_FD);
	void main_loop();
	void main_loop_recv(const int socket_FD, const int max_recv_per_socket,
		int & max_recv, bool & schedule_job);
	void main_loop_send(const int socket_FD, const int max_send_per_socket,
		int & max_send, bool & schedule_job);
	void process_connect_job();
	void process_finished_job();
	void process_force();
	void remove_socket(const int socket_FD);
	void remove_socket(boost::shared_ptr<sock> & S);
};
}//end of network namespace
#endif

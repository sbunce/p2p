//THREADSAFE, THREAD SPAWNING
#ifndef H_NEW_CONNECTION
#define H_NEW_CONNECTION

//boost
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>

//C
#include <errno.h>

//custom
#include "database.hpp"
#include "download_connection.hpp"
#include "p2p_buffer.hpp"
#include "settings.hpp"

//include
#include <atomic_int.hpp>
#include <thread_pool.hpp>

//networking
#ifdef WIN32
	#define FD_SETSIZE 1024 //max number of connections in FD_SET
	#include <winsock.h>
#else
	#include <arpa/inet.h>
	#include <netinet/in.h>
	#include <sys/socket.h>
#endif

//std
#include <list>
#include <map>
#include <string>

class new_connection : private boost::noncopyable
{
public:
	new_connection(
		fd_set & master_FDS_in,
		atomic_int<int> & connections_in,
		atomic_int<int> & max_connections_in,
		atomic_int<int> & FD_max_in,
		atomic_int<int> & localhost_socket_in
	);

	/*
	queue - queue download_connection
	*/
	void queue(download_connection DC);

private:
	//same variables that exist in p2p
	fd_set & master_FDS;
	atomic_int<int> & connections;
	atomic_int<int> & max_connections;
	atomic_int<int> & FD_max;
	atomic_int<int> & localhost_socket;

	/*
	Used by the known_unresponsive function to check for servers that have
	recently rejected a connection. The purpose of this is to avoid unnecessary
	connection attempts.
	*/
	boost::mutex KU_mutex;                           //mutex for Known_Unresponsive
	std::map<time_t,std::string> Known_Unresponsive; //time mapped to IP

	//holds connections which need to be made
	boost::mutex CQ_mutex;
	std::list<download_connection> Connection_Queue;

	/*
	Used by DC_block_concurrent to determine which servers are in progress of
	connecting.
	*/
	boost::mutex CCA_mutex;
	std::list<download_connection> Connection_Current_Attempt;

	/*
	add_unresponsive:
		Add IP to unresponsive map so connection attempts won't be wasted on it.
	block_concurrent:
		Only allow one IP at a time past (more detailed desc in function).
	check_unresponsive:
		Return true if IP on unresponsive list, else false.
	establish_outgoing_connection:
		Establishes new connections, adds connections to p2p_buffer.
	process_DC:
		Try to add DC to existing client_buffer else establish new connection.
	unblock:
		Works in conjunction with block_concurrent.
	*/
	void add_unresponsive(const std::string & IP);
	void block_concurrent(const download_connection & DC);
	bool check_unresponsive(const std::string & IP);
	void establish_outgoing_connection(download_connection DC);
	void process_DC(download_connection DC);
	void unblock(const download_connection & DC);

	database::table::blacklist DB_Blacklist;
	thread_pool Thread_Pool;
};
#endif

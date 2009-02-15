//THREADSAFE, THREAD SPAWNING
#ifndef H_P2P_NEW_CONNECTION
#define H_P2P_NEW_CONNECTION

//boost
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>

//C
#include <errno.h>

//custom
#include "atomic_int.hpp"
#include "database.hpp"
#include "download_connection.hpp"
#include "global.hpp"
#include "p2p_buffer.hpp"
#include "resolve.hpp"
#include "thread_pool.hpp"

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

class p2p_new_connection : private boost::noncopyable
{
public:
	p2p_new_connection(
		fd_set & master_FDS_in,
		atomic_int<int> & FD_max_in,
		atomic_int<int> & max_connections_in,
		atomic_int<int> & connections_in
	);
	~p2p_new_connection();

	/*
	queue - queue download_connection
	*/
	void queue(download_connection DC);

private:
	fd_set * master_FDS;               //pointer to master_FDS in client
	atomic_int<int> * FD_max;          //pointer to FD_max which exists in client
	atomic_int<int> * max_connections; //maximum number of connections allowed
	atomic_int<int> * connections;     //number of connections currently established

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
	add_unresponsive   - adds a IP to the unresponsive map so that connection attempts won't be wasted on it
	check_unresponsive - returns true if the IP is on the unresponsive list, else false
	block_concurrent   - only allows one IP at a time past (more detailed desc in function)
	main_thread        - checks if DCs queued, starts connection threads
	new_connection     - establishes new connections, adds connections to client_buffer
	process_DC         - process a DC, try to add it to an existing client_buffer else make new connection
	unblock            - works in conjunction with block_concurrent
	*/
	void add_unresponsive(const std::string & IP);
	bool check_unresponsive(const std::string & IP);
	void block_concurrent(const download_connection & DC);
	void main_thread();
	void new_connection(download_connection DC);
	void process_DC(download_connection DC);
	void unblock(const download_connection & DC);

	database::table::blacklist DB_Blacklist;
	thread_pool Thread_Pool;
};
#endif

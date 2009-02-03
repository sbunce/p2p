//THREADSAFE

#ifndef H_SERVER
#define H_SERVER

//boost
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>

//C
#include <errno.h>
#include <signal.h>

//custom
#include "atomic_int.hpp"
#include "database.hpp"
#include "global.hpp"
#include "rate_limit.hpp"
#include "server_buffer.hpp"
#include "server_index.hpp"
#include "upload_info.hpp"

//networking
#ifdef WIN32
	#define FD_SETSIZE 1024 //max number of connections in FD_SET
	#define MSG_NOSIGNAL 0  //do not signal SIGPIPE on send() error
	#define socklen_t int   //hack for little API difference on windows
	#include <winsock.h>
#else
	#include <arpa/inet.h>
	#include <netinet/in.h>
	#include <sys/socket.h>
#endif

//std
#include <ctime>
#include <deque>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <sstream>
#include <string>
#include <vector>

class server : private boost::noncopyable
{
public:
	server();
	~server();

	/*
	current_uploads     - returns current upload information
	get_max_connections - returns maximum connections server will accept
	get_share_directory - returns the location to be shared/indexed
	get_upload_rate     - returns the download speed limit (B/s)
	is_indexing         - returns true if the server is indexing files
	set_max_connections - sets maximum connections server will accept
	set_share_directory - sets the share directory
	set_max_upload_rate - sets a new upload rate limit (B/s)
	total_rate          - returns total upload rate (B/s)
	*/
	void current_uploads(std::vector<upload_info> & info);
	unsigned get_max_connections();
	std::string get_share_directory();
	unsigned get_upload_rate();
	bool is_indexing();
	void set_max_connections(const unsigned & max_connections_in);
	void set_share_directory(const std::string & share_directory);
	void set_max_upload_rate(unsigned upload_rate);
	unsigned total_rate();

private:
	boost::thread main_thread;

	atomic_int<int> connections;     //currently established connections
	atomic_int<int> max_connections; //connection limit

	//select() related
	fd_set master_FDS; //master file descriptor set
	fd_set read_FDS;   //set when socket can read without blocking
	fd_set write_FDS;  //set when socket can write without blocking
	int FD_max;        //holds the number of the maximum socket

	/*
	check_blacklist - disconects servers which have been blacklisted
	disconnect      - disconnect client and remove socket from master set
	main_thread     - where the party is at
	new_conn        - sets up socket for client
	process_request - adds received bytes to buffer and interprets buffer
	*/
	void check_blacklist(int & blacklist_state);
	void disconnect(const int & socketfd);
	void main_loop();
	void new_connection(const int & listener);
	void process_request(const int & socket_FD, char * recv_buff, const int & n_bytes);

	database::table::blacklist DB_Blacklist;
	database::table::preferences DB_Preferences;
};
#endif

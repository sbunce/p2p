#ifndef H_SERVER
#define H_SERVER

//boost
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>

//C
#include <errno.h>
#include <signal.h>

//custom
#include "DB_server_preferences.h"
#include "global.h"
#include "server_buffer.h"
#include "server_index.h"
#include "server_protocol.h"
#include "speed_calculator.h"

//networking
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

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

class server
{
public:
	server();
	~server();

	/*
	get_max_connections - returns maximum connections server will accept
	set_max_connections - sets maximum connections server will accept
	get_share_directory - returns the location to be shared/indexed
	set_share_directory - sets the share directory
	get_speed_limit     - returns the download speed limit (kilobytes/second)
	set_speed_limit     - sets a new download speed limit (kilobytes/second)
	get_upload_info     - retrieves information on all current uploads
	                      returns true if upload_info updated
	get_total_speed     - returns the total speed of all uploads (bytes per second)
	is_indexing         - returns true if the server is indexing files
	start               - starts the server
	stop                - stops the server (blocks until server shut down)
	*/
	int get_max_connections();
	void set_max_connections(int max_connections);
	std::string get_share_directory();
	void set_share_directory(const std::string & share_directory);
	std::string get_speed_limit();
	void set_speed_limit(const std::string & speed_limit);
	int get_total_speed();
//	bool get_upload_info(std::vector<info_buffer> & upload_info);
	bool is_indexing();
	void start();
	void stop();

private:
	std::map<int, server_buffer *> Server_Buffer;

	volatile bool stop_threads; //if true this will trigger thread termination
	volatile int threads;       //how many threads are currently running

	//how many are connections currently established
	int connections;

	//select() related
	fd_set master_FDS;   //master file descriptor set
	fd_set read_FDS;     //set when socket can read without blocking
	fd_set write_FDS;    //set when socket can write without blocking
	int FD_max;          //holds the number of the maximum socket

	//counter for how many sends need to be done
	volatile int send_pending;

	/*
	disconnect         - disconnect client and remove socket from master set
	main_thread        - where the party is at
	new_conn           - sets up socket for client
	                   - returns true if connection suceeded
	process_request    - interprets the request from a client and buffers it
	*/
	void disconnect(const int & socketfd);
	void main_thread();
	bool new_conn(const int & listener);
	void process_request(const int & socketfd, char recvBuffer[], const int & n_bytes);

	DB_server_preferences DB_Server_Preferences;
	server_index Server_Index;
	server_protocol Server_Protocol;
	speed_calculator Speed_Calculator;
};
#endif

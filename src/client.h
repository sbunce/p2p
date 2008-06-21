#ifndef H_CLIENT
#define H_CLIENT

//boost
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

//C
#include <errno.h>

//custom
#include "client_buffer.h"
#include "DB_download.h"
#include "DB_client_preferences.h"
#include "DB_search.h"
#include "download.h"
#include "download_conn.h"
#include "download_info.h"
#include "download_factory.h"
#include "global.h"
#include "speed_calculator.h"
#include "thread_pool.h"

//boost
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/bind.hpp>

//networking
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

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

class client
{
public:
	client();
	~client();
	/*
	current_downloads      - returns download info for all downloads
	get_max_connections    - returns maximum connections client will make
	set_max_connections    - sets maximum connections client will make
	get_download_directory - returns the location where downloads are saved to
	set_download_directory - sets the download directory
	get_speed_limit        - returns the download speed limit (kilobytes/second)
	set_speed_limit        - sets a new download speed limit (kilobytes/second)
	search                 - populates info with download_info that match the search_word
	start                  - start the threads needed for the client
	stop                   - stops all threads, must be called before destruction
	start_download         - schedules a download_file to be started
	stop_download          - marks a download as completed so it will be stopped
	total_speed            - returns the total download speed(in bytes per second)
	*/
	void current_downloads(std::vector<download_info> & info);
	int get_max_connections();
	void set_max_connections(int max_connections);
	std::string get_download_directory();
	void set_download_directory(const std::string & download_directory);
	std::string get_speed_limit();
	void set_speed_limit(const std::string & speed_limit);
	void search(std::string search_word, std::vector<download_info> & Search_Info);
	void start();
	void stop();
	bool start_download(download_info & info);
	void stop_download(std::string hash);
	int total_speed();

private:
	volatile bool stop_threads; //if true this will trigger thread termination
	volatile int threads;       //how many threads are currently running (currently just one in main_thread())

	//holds connections which need to be made
	boost::mutex CQ_mutex;
	std::list<download_conn *> Connection_Queue;

	//holds connections which are in progress of connecting
	boost::mutex CCA_mutex;
	std::list<download_conn *> Connection_Current_Attempt;

	/*
	Socket number mapped to client_buffer.

	This should only be accessed by the thread active in main_thread().
	*/
	std::map<int, client_buffer *> Client_Buffer;

	//networking related
	fd_set master_FDS; //master file descriptor set
	fd_set read_FDS;   //holds what sockets are ready to be read
	fd_set write_FDS;  //holds what sockets can be written to without blocking
	int FD_max;        //holds the number of the maximum socket

	/*
	When this is zero there are no pending requests to send. This exists for the
	purpose of having an alternate select() call that doesn't involve write_FDS
	because using write_FDS hogs CPU. When the value referenced by this equals 0
	then there are no sends pending and write_FDS doesn't need to be used. These
	are given to the client_buffer elements so they can increment it.
	*/
	volatile int * send_pending;

	//used to initiate new connections
	thread_pool<client> Thread_Pool;

	/*
	Used by the known_unresponsive function to check for servers that have
	recently rejected a connection. The purpose of this is to avoid unnecessary
	connection attempts.
	*/
	boost::mutex KU_mutex;                           //mutex for Known_Unresponsive
	std::map<time_t,std::string> Known_Unresponsive; //time mapped to IP

	/*
	check_timeouts      - checks all servers to see if they've timed out and removes/disconnects if they have
	DC_add_existing     - tries to add a DC to an existing client_buffer
	                      returns true if succeeded, else false
	DC_block_concurrent - will block the second or greater DC that passes with the same server_IP
	                      lets a new DC by whenever the DC blocking is unblocked with DC_unblock
	DC_unblock          - allows DC_block_concurrent to release a DC being blocked
	disconnect          - disconnects a socket, modifies Client_Buffer
	known_unresponsive  - returns true if a connection to the server failed within global::UNRESPONSIVE_TIMEOUT time
	main_thread         - main client thread that sends/receives data and triggers events
	new_conn            - connects to the server specified by the DC
	prepare_requests    - touches each Client_Buffer element to trigger new requests(if needed)
	process_CQ          - connects to servers for DC's or initiates adding a DC to existing client_buffer
	                      one Thread_Pool job should be scheduled for each DC in Connection_Queue
	remove_complete     - removes downloads that are complete(or stopped)
	*/
	void check_timeouts();
	bool DC_add_existing(download_conn * DC);
	void DC_block_concurrent(download_conn * DC);
	void DC_unblock(download_conn * DC);
	inline void disconnect(const int & socket_FD);
	bool known_unresponsive(const std::string & IP);
	void main_thread();
	void new_conn(download_conn * DC);
	void prepare_requests();
	void process_CQ();
	void remove_complete();
	void remove_complete_0(std::list<download *> & complete);
	void remove_complete_1(std::list<download *> & complete);
	void remove_complete_2(std::list<download *> & complete);
	void remove_complete_3(std::list<download *> & complete);

	sha SHA;
	DB_download DB_Download;
	DB_client_preferences DB_Client_Preferences;
	DB_search DB_Search;
	download_factory Download_Factory;
	speed_calculator Speed_Calculator;
};
#endif

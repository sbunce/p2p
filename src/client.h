#ifndef H_CLIENT
#define H_CLIENT

//boost
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

//custom
#include "client_buffer.h"
#include "client_new_connection.h"
#include "DB_blacklist.h"
#include "DB_download.h"
#include "DB_client_preferences.h"
#include "DB_search.h"
#include "download.h"
#include "download_connection.h"
#include "download_info.h"
#include "download_factory.h"
#include "global.h"
#include "speed_calculator.h"

//boost
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/bind.hpp>

//networking
#ifdef WIN32
#include <winsock.h>
#define socklen_t int
#define MSG_NOSIGNAL 0
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
	get_speed_limit        - returns the download speed limit (bytes/second)
	set_speed_limit        - sets a new download speed limit (bytes/second)
	search                 - populates info with download_info that match the search_word
	start                  - start the threads needed for the client
	stop                   - stops all threads, must be called before destruction
	start_download         - schedules a download_file to be started
	stop_download          - marks a download as completed so it will be stopped
	total_speed            - returns the total download speed(in bytes per second)
	*/
	void current_downloads(std::vector<download_info> & info);
	int get_max_connections();
	void set_max_connections(int max_connections_in);
	std::string get_download_directory();
	void set_download_directory(const std::string & download_directory);
	std::string get_speed_limit();
	void set_speed_limit(const std::string & speed_limit);
	void search(std::string search_word, std::vector<download_info> & Search_Info);
	void start_download(const download_info & info);
	void stop_download(std::string hash);
	int total_speed();

private:
	volatile bool stop_threads; //if true this will trigger thread termination
	volatile int threads;       //how many threads are currently running

	volatile int connections;     //currently established connections
	volatile int max_connections; //connection limit

	/*
	Holds download_info for downloads that need to be started. The download_info
	is put in this container and later processed by main_thread(). This is done
	because the logic which starts the download takes long enough to where the
	GUI function which calls it blocks for an unacceptible amount of time.
	*/
	boost::mutex PD_mutex;
	std::list<download_info> Pending_Download;

	//networking related
	fd_set master_FDS; //master file descriptor set
	fd_set read_FDS;   //holds what sockets are ready to be read
	fd_set write_FDS;  //holds what sockets can be written to without blocking
	int FD_max;        //holds the number of the maximum socket

	//passed to DB_Blacklist to check for updates to blacklist
	int blacklist_state;

	/*
	check_blacklist         - checks if blacklist was updated, if so checks connected IP's to see if any are blacklisted
	check_timeouts          - checks all servers to see if they've timed out and removes/disconnects if they have
	disconnect              - disconnects a socket, modifies Client_Buffer
	main_thread             - main client thread that sends/receives data and triggers events
	prepare_requests        - touches each Client_Buffer element to trigger new requests(if needed)
	reconnect_unfinished    - called by main_thread() to resume unfinished downloads
	                          called before main_thread accesses shared objects so no mutex needed
	remove_complete         - removes downloads that are complete(or stopped)
	start_download_process  - starts a download added by start_download()
	start_pending_downloads - called by main_thread() to start downloads added by start_download()
	transition_download     - passes terminating download to download_factory, starts downloads download_factory may return
	*/
	void check_blacklist();
	void check_timeouts();
	inline void disconnect(const int & socket_FD);
	void main_thread();
	void reconnect_unfinished();
	void remove_complete();
	void start_download_process(const download_info & info);
	void start_pending_downloads();
	void transition_download(download * Download_Stop);

	client_new_connection Client_New_Connection;
	DB_download DB_Download;
	DB_client_preferences DB_Client_Preferences;
	DB_search DB_Search;
	download_factory Download_Factory;
	speed_calculator Speed_Calculator;
	sha SHA;
};
#endif

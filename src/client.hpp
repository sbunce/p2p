//THREADSAFE, CTOR THREAD SPAWNING

#ifndef H_CLIENT
#define H_CLIENT

//boost
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>

//custom
#include "atomic_int.hpp"
#include "client_buffer.hpp"
#include "client_new_connection.hpp"
#include "client_server_bridge.hpp"
#include "database.hpp"
#include "download.hpp"
#include "download_connection.hpp"
#include "download_info.hpp"
#include "download_status.hpp"
#include "download_factory.hpp"
#include "global.hpp"
#include "locking_shared_ptr.hpp"
#include "number_generator.hpp"
#include "rate_limit.hpp"
#include "speed_calculator.hpp"

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
#include <limits>
#include <list>
#include <map>
#include <sstream>
#include <string>
#include <vector>

class client : private boost::noncopyable
{
public:
	client();
	~client();
	/*
	current_downloads      - returns download info for all downloads or
	                         if hash set info only retrieved for download that corresponds to hash
	                         WARNING: if no hash found info will be empty
	file_info              - returns specific information about file that corresponds to hash (for currently running downloads)
	                         WARNING: Expensive call that includes database access, do not poll this.
	                                  A good possible use of this is to get info when opening download info tabs.
	get_max_connections    - returns maximum connections client will make
	get_download_directory - returns the location where downloads are saved to
	get_download_rate        - returns the download speed limit (bytes/second)
	prime_count            - returns the number of primes cached for Diffie-Hellman
	search                 - populates info with download_status that match the search_word
	set_max_connections    - sets maximum connections client will make
	set_download_directory - sets the download directory
	set_max_download_rate  - sets a new download rate limit (B/s)
	start_download         - schedules a download_file to be started
	stop_download          - marks a download as completed so it will be stopped
	total_rate             - returns the total download rate(B/s)
	*/
	void current_downloads(std::vector<download_status> & info, std::string hash = "");
	bool file_info(const std::string & hash, std::string & path, boost::uint64_t & tree_size, boost::uint64_t & file_size);
	unsigned get_max_connections();
	std::string get_download_directory();
	unsigned get_download_rate();
	unsigned prime_count();
	void search(std::string search_word, std::vector<download_info> & Search_Info);
	void set_max_connections(const unsigned & max_connections_in);
	void set_download_directory(const std::string & download_directory);
	void set_max_download_rate(unsigned download_rate);
	void start_download(const download_info & info);
	void stop_download(std::string hash);
	unsigned total_rate();

private:
	//thread for main_loop
	boost::thread main_thread;

	atomic_int<int> connections;     //currently established connections
	atomic_int<int> max_connections; //connection limit

	/*
	Holds download_info for downloads that need to be started. The download_info
	is put in this container and later processed by main_thread(). This is done
	because the logic which starts the download takes long enough to where the
	GUI function which calls it blocks for an unacceptible amount of time.
	*/
	locking_shared_ptr<std::list<download_info> > Pending_Download;

	//networking related
	fd_set master_FDS;      //master file descriptor set
	fd_set read_FDS;        //holds what sockets are ready to be read
	fd_set write_FDS;       //holds what sockets can be written to without blocking
	atomic_int<int> FD_max; //number of the highest socket

	/*
	check_blacklist         - checks if blacklist was updated, if so checks connected IP's to see if any are blacklisted
	check_timeouts          - checks all servers to see if they've timed out and removes/disconnects if they have
	disconnect              - disconnects a socket, modifies Client_Buffer
	main_thread             - main client thread that sends/receives data and triggers events
	prepare_requests        - touches each Client_Buffer element to trigger new requests(if needed)
	reconnect_unfinished    - called by main_thread() to resume unfinished downloads
	                          called before main_thread accesses shared objects so no mutex needed
	remove_complete         - removes downloads that are complete(or stopped)
	remove_empty            - remove any empty client_buffers as they are no longer needed
	start_download_process  - starts a download added by start_download()
	start_pending_downloads - called by main_thread() to start downloads added by start_download()
	transition_download     - passes terminating download to download_factory, starts downloads download_factory may return
	*/
	void check_blacklist(int & blacklist_state);
	void check_timeouts();
	inline void disconnect(const int & socket_FD);
	void main_loop();
	void reconnect_unfinished();
	void remove_complete();
	void remove_empty();
	void start_download_process(const download_info & info);
	void start_pending_downloads();
	void transition_download(locking_shared_ptr<download> Download_Stop);

	client_new_connection Client_New_Connection;
	database::table::blacklist DB_Blacklist;
	database::table::download DB_Download;
	database::table::preferences DB_Preferences;
	database::table::search DB_Search;
	download_factory Download_Factory;
	sha SHA;
};
#endif

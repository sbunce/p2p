//THREADSAFE, CTOR THREAD SPAWNING
#ifndef H_P2P
#define H_P2P

//boost
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>

//custom
#include "database.hpp"
#include "download.hpp"
#include "download_connection.hpp"
#include "download_info.hpp"
#include "download_status.hpp"
#include "download_factory.hpp"
#include "p2p_buffer.hpp"
#include "new_connection.hpp"
#include "number_generator.hpp"
#include "rate_limit.hpp"
#include "settings.hpp"
#include "share_index.hpp"

//include
#include <atomic_int.hpp>
#include <speed_calculator.hpp>

//networking
#ifdef WIN32
	#define FD_SETSIZE 1024 //max number of connections in FD_SET
	#define MSG_NOSIGNAL 0  //do not signal SIGPIPE on send() error
	#define socklen_t int   //hack for API difference
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

class p2p : private boost::noncopyable
{
public:
	p2p();
	~p2p();
	/*
	current_downloads:
		Returns download info for all downloads or if hash set info is only
		retrieved for one download.
		WARNING: If no hash found info will be empty.
	current_uploads:
		Populates info with upload_info for all uploads.
	file_info:
		Returns specific information about file that corresponds to hash (for
		currently running downloads).
		WARNING: Expensive call that includes database access, do not poll this.
	get_max_connections:
		Returns connection limit.
	get_download_directory:
		Returns the location where downloads are saved to.
	get_max_download_rate:
		Returns the download rate limit.
	pause_download:
		Pause a download.
	prime_count:
		Returns size of prime cache used for key-exchanges.
	remove_download:
		Stops/Removes a running download.
	search:
		Populates the info vector with results to search.
	set_max_connections:
		Sets connection limit. Disconnects servers at random if over the limit.
	set_download_directory:
		Set location where downloads are to be saved to. Does not change location
		existing downloads are to be saved to.
	set_max_download_rate:
		Sets the upload rate limit.
	start_download:
		Starts a download.
	download_rate:
		Returns average download rate for everything (excluding localhost).
	upload_rate:
		Returns average upload rate for everything (excluding localhost).
	*/
	void current_downloads(std::vector<download_status> & info, std::string hash = "");
	void current_uploads(std::vector<upload_info> & info);
	bool file_info(const std::string & hash, std::string & path, boost::uint64_t & tree_size, boost::uint64_t & file_size);
	unsigned get_max_connections();
	std::string get_download_directory();
	std::string get_share_directory();
	unsigned get_max_download_rate();
	unsigned get_max_upload_rate();
	void pause_download(const std::string & hash);
	unsigned prime_count();
	bool is_indexing();
	void remove_download(const std::string & hash);
	void search(std::string search_word, std::vector<download_info> & Search_Info);
	void set_max_connections(const unsigned & max_connections_in);
	void set_download_directory(const std::string & download_directory);
	void set_share_directory(const std::string & share_directory);
	void set_max_download_rate(unsigned download_rate);
	void set_max_upload_rate(unsigned upload_rate);
	void start_download(const download_info & info);
	unsigned download_rate();
	unsigned upload_rate();

private:
	//thread for main_loop
	boost::thread main_thread;

	fd_set master_FDS;                //master file descriptor set
	fd_set read_FDS;                  //what sockets can read non-blocking
	fd_set write_FDS;                 //what sockets can write non-blocking
	atomic_int<int> connections;      //currently established connections
	atomic_int<int> max_connections;  //connection limit
	atomic_int<int> FD_max;           //number of the highest socket
	atomic_int<int> localhost_socket; //socket of localhost, or -1 if not connected

	//socket mapped to time at which the socket last had activity
	std::map<int, time_t> Active;

	/*
	Holds download_info for downloads that need to be started. The download_info
	is put in this container and later processed by main_thread(). This is done
	because the logic which starts the download takes long enough to where the
	GUI function which calls it blocks for an unacceptible amount of time.
	*/
	boost::mutex PD_mutex; //for all access to Pending_Download
	std::list<download_info> Pending_Download;

	/*
	check_blacklist:
		Disconnects IP's that were blacklisted.
	check_timeouts:
		Check for timed out sockets.
	disconnect:
		Disconnect a socket.
	establish_incoming_connection:
		Sets up a incoming connection.
	main_thread:
		Main client thread that sends/receives data and triggers events.
	prepare_requests:
		Generates new requests. Called before checking socket sets.
	reconnect_unfinished:
		Resumes downloads upon program start.
	remove_complete:
		Removes downloads that are complete(or stopped).
	start_download_process:
		Starts download added by start_download().
	start_pending_downloads:
		Called by main_thread() to start downloads added by start_download()
	transition_download:
		Passes terminating download to download_factory, starts downloads download_factory may return
	update_active:
		Updates the time at which the socket last had activity. Do not call this
		function on the listener socket.
	*/
	void check_blacklist(int & blacklist_state);
	void check_timeouts();
	void disconnect(const int & socket_FD);
	void establish_incoming_connection(const int & listener);
	void main_loop();
	void reconnect_unfinished();
	void remove_complete();
	int setup_listener();
	void start_download_process(const download_info & info);
	void start_pending_downloads();
	void transition_download(boost::shared_ptr<download> Download_Stop);
	void update_active(const int & socket_FD);

	database::table::blacklist DB_Blacklist;
	database::table::download DB_Download;
	database::table::preferences DB_Preferences;
	database::table::search DB_Search;
	download_factory Download_Factory;
	new_connection New_Connection;
	sha SHA;
};
#endif

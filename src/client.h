#ifndef H_CLIENT
#define H_CLIENT

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

//contains enum type for error codes select() might return
#include <errno.h>

//std
#include <ctime>
#include <list>
#include <map>
#include <string>
#include <vector>

#include "atomic.h"
#include "client_buffer.h"
#include "client_index.h"
#include "download.h"
#include "download_conn.h"
#include "download_file.h"
#include "download_file_conn.h"
#include "exploration.h"
#include "global.h"

class client
{
public:
	//used to pass information to user interface
	class info_buffer
	{
	public:
		std::string hash;
		std::vector<std::string> server_IP;
		std::string file_name;
		int file_size;        //bytes
		int speed;            //bytes per second
		int percent_complete; //0-100
	};

	client();
	~client();
	/*
	get_download_info - returns download info for all files in sendBuffer
	                  - returns false if no downloads
	get_total_speed   - returns the total download speed(in bytes per second)
	start             - start the threads needed for the client
	stop              - stops all threads, must be called before destruction
	start_download    - schedules a download_file to be started
	stop_download     - marks a download as completed so it will be stopped
	*/
	bool get_download_info(std::vector<info_buffer> & download_info);
	int get_total_speed();
	void start();
	void stop();
	bool start_download(exploration::info_buffer info);
	void stop_download(const std::string & hash);

private:
	atomic<bool> stop_threads; //if true this will trigger thread termination
	atomic<int> threads;       //how many threads are currently running

	//holds the current time(set in main_thread), used for server timeouts
	time_t current_time;
	time_t previous_time;

	/*
	Holds connections which need to be made. If multiple connections to the same
	server need to be made they're put in the same inner list.
	*/
	//std::list<pending_connection *> Pending_Connection;
	std::list<download_conn *> Pending_Connection;

	sha SHA;                   //creates messageDigests
	client_index Client_Index; //gives client access to the database

	/*
	All the information relevant to each socket accessible with the socket fd int
	example: Client_Buffer[socket_FD].something.

	Download_Buffer stores all the downloads contained within the Client_Buffer in
	no particular order.

	Access to both of these must be locked with the same mutex
	(CB_D_mutex).
	*/
	std::map<int, client_buffer *> Client_Buffer;
	std::list<download *> Download_Buffer;

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
	atomic<int> * send_pending;

	//true if a download is complete, handed to all downloads
	atomic<bool> * download_complete;

	/*
	check_timeouts     - checks all servers to see if they've timed out and removes/disconnects if they have
	disconnect         - disconnects a socket, modifies Client_Buffer
	main_thread        - main client thread that sends/receives data and triggers events
	new_conn           - create a connection with a server, modifies Client_Buffer
	prepare_requests   - touches each Client_Buffer element to trigger new requests(if needed)
	remove_complete    - removes downloads that are complete(or stopped)
	*/
	void check_timeouts();
	inline void disconnect(const int & socket_FD);
	void main_thread();
	void new_conn(download_conn * DC);
	void prepare_requests();
	void remove_complete();

	//each mutex names the object it locks in the format boost::mutex <object>Mutex
	boost::mutex CB_D_mutex; //for both Client_Buffer and Download_Buffer
	boost::mutex DC_mutex;   //locks access to download_conn instances

	/*
	Locks access to everything NOT stop_threads. This is used by stop() to terminate
	all threads.
	*/
	boost::mutex stop_mutex;
};
#endif

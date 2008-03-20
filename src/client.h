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
#include <netdb.h>

//contains error codes select() might return
#include <errno.h>

//std
#include <ctime>
#include <list>
#include <map>
#include <string>
#include <vector>

//custom
#include "client_buffer.h"
#include "DB_access.h"
#include "download.h"
#include "download_conn.h"
#include "download_prep.h"
#include "global.h"
#include "speed_calculator.h"
#include "thread_pool.h"

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
	bool start_download(DB_access::download_info_buffer & info);
	void stop_download(const std::string & hash);

private:
	volatile bool stop_threads; //if true this will trigger thread termination
	volatile int threads;       //how many threads are currently running

	//used for timeouts
	time_t current_time;
	time_t previous_time;

	//holds connections which need to be made
	std::queue<download_conn *> Connection_Queue;
	std::list<download_conn *> Connection_Current_Attempt;

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
	volatile int * send_pending;

	//true if a download is complete, handed to all downloads
	volatile bool * download_complete;

	//used to initiate new connections
	thread_pool<client> Thread_Pool;

	/*
	check_timeouts   - checks all servers to see if they've timed out and removes/disconnects if they have
	disconnect       - disconnects a socket, modifies Client_Buffer
	main_thread      - main client thread that sends/receives data and triggers events
	new_conn         - create a connection with a server, modifies Client_Buffer
	new_conn_block_concurrent - if two download_conn's with the same IP pass the second one gets blocked until new_conn_unblock called with first one
	prepare_requests - touches each Client_Buffer element to trigger new requests(if needed)
	remove_complete  - removes downloads that are complete(or stopped)
	*/
	void check_timeouts();
	inline void disconnect(const int & socket_FD);
	void main_thread();
	void new_conn();
	void new_conn_block_concurrent(download_conn * DC);
	void new_conn_unblock(download_conn * DC);
	void prepare_requests();
	void remove_complete();

	//each mutex names the object it locks in the format boost::mutex <object>Mutex
	boost::mutex CB_D_mutex; //for both Client_Buffer and Download_Buffer
	boost::mutex DC_mutex;   //locks access to download_conn instances

	sha SHA;
	DB_access DB_Access;
	download_prep Download_Prep;
	speed_calculator Speed_Calculator;
};
#endif

//NOT-THREADSAFE

/*
The NOT-THREADSAFE functions are annotated at the top of the class definition.

The client_buffer was intentionally made NOT-THREADSAFE to avoid copying a
buffer. However, it's simple to follow the imposed constrains due to this.
*/

#ifndef H_CLIENT_BUFFER
#define H_CLIENT_BUFFER

//boost
#include <boost/thread.hpp>
#include <boost/utility.hpp>

//custom
#include "convert.hpp"
#include "database.hpp"
#include "download.hpp"
#include "download_status.hpp"
#include "encryption.hpp"
#include "global.hpp"
#include "locking_shared_ptr.hpp"

//std
#include <ctime>
#include <deque>
#include <iostream>
#include <list>
#include <map>
#include <set>

class client_buffer : private boost::noncopyable
{
public:
	/*
	The client_buffer is used in a std::map. If std::map::[] is ever used to
	locate a client_buffer that doesn't exist the default ctor will be called and
	the program will be killed.
	*/
	client_buffer();

	~client_buffer();


	/****************************************************************************
	BEGIN NOT-THREADSAFE
	WARNING: These functions should only be called by main_thread in the client.
	****************************************************************************/
	/*
	Causes all client_buffers to query their downloads to see if they need to make
	new requests.
	WARNING: This function touches send_buff which get_send_buff() returns.
	*/
	static void generate_requests();

	/*
	Returns the client_buffer send_buff that corresponds to socket_FD.
	WARNING: Returns a reference to the buffer that generate_requests() modifies.
	*/
	static std::string & get_send_buff(const int & socket_FD);
	/****************************************************************************
	END NOT-THREADSAFE
	****************************************************************************/


	/*
	Try to add download to an existing client_buffer. If client_buffer is found
	then return true. If no client_buffer is found false will be returned and a
	new connection should be made.
	*/
	static bool add_connection(download_connection & DC);

	/*
	Add a download to the client_buffer. Doesn't associate the download with any
	instantiation of client_buffer.
	*/
	static void add_download(locking_shared_ptr<download> Download);

	/*
	Populates the info vector with download information for all downloads running
	in all client_buffers. The vector passed in is cleared before download info is
	added.
	*/
	static void current_downloads(std::vector<download_status> & info, std::string hash);

	/*
	Erases an element from Client_Buffer associated with socket_FD. This should be
	called whenever a socket is disconnected.
	*/
	static void erase(const int & socket_FD);

	/*
	Returns the counter that indicates whether a send needs to be done. If it is
	zero no send needs to be done. Every +1 above zero it is indicates a send_buff
	has something in it.
	*/
	static int get_send_pending();

	/*
	Check if a download is running. Returns true if yes, else false.
	*/
	static bool is_downloading(locking_shared_ptr<download> Download);

	/*
	Check by hash if a download is running. Returns true if yes, else false.
	*/
	static bool is_downloading(const std::string & hash);

	/*
	Populates the complete list with pointers to downloads which are completed.
	WARNING: Do not do anything with these pointers but compare them by value. Any
	         dereferencing of these pointers is NOT thread safe.
	*/
	static void find_complete(std::vector<locking_shared_ptr<download> > & complete);

	/*
	Removes all empty client_buffers and returns their socket numbers which need
	to be disconnected.
	*/
	static void find_empty(std::vector<int> & disconnect_sockets);

	/*
	Removes client_buffers that have timed out. Puts sockets to disconnect in
	timed_out vector.
	*/
	static void find_timed_out(std::vector<int> & timed_out);

	/*
	Creates new client_buffer. Registers the download with the client_buffer.
	Registers the download_conn with the download.

	Precondition: add_download must be called for the download contained within
	the DC.

	Returns true if the download associated with the DC was found, else false.
	If the download is not found the connection can not be added.
	*/
	static void new_connection(const download_connection & DC);

	/*
	Should be called after sending data. This function erases the sent bytes and
	evaluates whether or not the send_buff was emptied. If it was then it
	decrements send_pending.
	*/
	static void post_send(const int & socket_FD, const int & n_bytes);

	/*
	Append bytes to a buffer for a specified socket.
	*/
	static void recv_buff_append(const int & socket_FD, char * buff, const int & n_bytes);

	/*
	Remove the download from the Unique_Download container and from any instantiation
	of the client_buffer.
	*/
	static void remove_download(locking_shared_ptr<download> Download);

	//stops the download associated with hash
	static void stop_download(const std::string & hash);

private:
	//only the client_buffer can instantiate itself
	client_buffer(const int & socket_in, const std::string & IP_in);

	//used to protect all public static functions
	static boost::mutex Mutex;

	/*
	All currently running downloads. All use of Unique_Download should be locked with UD_mutex.
	*/
	static std::set<locking_shared_ptr<download> > Unique_Download;

	/*
	Socket mapped to client_buffer. All use of Client_Buffer should be locked with CB_mutex.
	This includes all calls to client_buffer member functions.
	*/
	static std::map<int, boost::shared_ptr<client_buffer> > Client_Buffer;

	/*
	When this is zero there are no pending requests to send. This exists for the
	purpose of having an alternate select() call that doesn't involve write_FDS
	because using write_FDS hogs CPU. When the value referenced by this equals 0
	then there are no sends pending and write_FDS doesn't need to be used. These
	are given to the client_buffer elements so they can increment it.
	*/
	static int send_pending;

	//main send/recv buffers
	std::string recv_buff;
	std::string send_buff;

	/*
	Used when parsing the recv_buff. This placeholder indicates how many bytes in
	the recv_buff have already been seen in previous calls to post_recv(). This
	is used to update the speed_calculators in the downloads.
	*/
	unsigned bytes_seen;

	/*
	The Download container is effectively a ring. The rotate_downloads() function will move
	Download_iter through it in a circular fashion.
	*/
	std::list<locking_shared_ptr<download> > Download;                //all downloads that this client_buffer is serving
	std::list<locking_shared_ptr<download> >::iterator Download_iter; //last download a request was gotten from

	std::string IP;   //IP associated with this client_buffer
	int socket;       //socket number of this element
	time_t last_seen; //used for timeout

	//this will be adjusted up or down depending on whether the server needs it
	int max_pipeline_size;

	class pending_response
	{
	public:
		pending_response(){}

		//copy ctor
		pending_response(
			const pending_response & PR
		):
			Mode(PR.Mode),
			expected(PR.expected),
			Download(PR.Download)
		{}

		//mode determines how to parse the response
		download::mode Mode;

		//possible responses paired with size of the possible response
		std::vector<std::pair<char, int> > expected;
		locking_shared_ptr<download> Download;
	};

	/*
	Past requests are stored here. The front of this queue will contain the oldest
	requests. When a response comes in it will correspond to the pending_response
	in the front of this queue.
	*/
	std::deque<pending_response> Pipeline;

	/*
	The number of slots in use by the downloads contained within this client_buffer.
	This is passed to downloads by reference so they may modify it. Checks exist
	in the client_buffer such that if this value goes negative or above 255 the
	program will be terminated.
	*/
	int slots_used;

	/*
	recv_buff_append   - appends bytes to recv_buff and calls post_recv
	empty              - return true if client_buffer contains no downloads
	post_recv          - does actions which may need to be done after a recv
	post_send          - should be called after every send
	prepare_request    - if another request it needed rotate downloads and get a request
	register_download  - associate a download with this client_buffer
	rotate_downloads   - moves Download_iter through Download in a circular fashion
	                     returns true if looped back to beginning
	terminate_download - removes the download which corresponds to hash
	unregister_all     - unregisters the connection with all available downloads
	                     called from destructor to unregister from all downloads
	                     when the client_buffer is destroyed
	*/
	void recv_buff_append(char * buff, const int & n_bytes);
	bool empty();
	void post_recv();
	void prepare_request();
	void register_download(locking_shared_ptr<download> new_download);
	bool rotate_downloads();
	void terminate_download(locking_shared_ptr<download> term_DL);
	void unregister_all();

	/*
	This is set to true when key exchange happening. While true the client_buffer
	will not get requests from downloads.
	*/
	bool exchange_key;
	std::string remote_result; //holds remote result for key exchange
	encryption Encryption;

	database::table::blacklist DB_Blacklist;
};
#endif

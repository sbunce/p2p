//THREADSAFE

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
	~client_buffer();

	/*
	add_connection    - Adds connection to client_buffer, returns true if success.
	add_download      - Adds download to unique download set. This is needed because
	                    it's possible that a download can be started with no connected
	                    servers.
	current_downloads - Populates the info vector with download status information.
	                    If hash is specified then only information from the download
	                    which corresponds to hash is added to the vector.
	                    Note: the info vector is cleared before it's populated
	erase             - Erases client_buffer associated with socket_FD. This should
	                    be called whenever a socket is disconnected.
	generate_requests - Causes all client_buffers to get requests from their downloads.
	                    Note: this is called by client::main_thread before socket
	                    servicing loop in main_loop().
	get_empty         - Removes all empty client_buffers and populates disconnect
	                    vector with sockets which need to be disconnected.
	get_complete      - Populates complete vector with complete downloads. Removes
	                    complete downloads from all client_buffers and Unique_Download.
	get_send_buff     - Copies max_bytes from the client_buffer send_buff in to the
	                    destination buff. True is returned if destination is not
	                    empty after function completes.
	                    Note 1: max_bytes should be as low as possible to avoid
	                            unnecessary copying.
	                    Note 2: Once a send is done, call post_send to erase the
	                            bytes sent from the client_buffer send_buff.
	get_send_pending  - Returns counter that indicates how many sockets have bytes
	                    to send. Used by client to check if write_FDS set should be
	                    used.
	get_timed_out     - Removes client_buffers that timed out and populates timed_out
	                    vector with sockets which should be disconnected.
	is_downloading    - Returns true if download specified by pointer or hash is running.
	new_connection    - Adds download to client_buffer. Creates new client_buffer
	                    if necessary. Silently fails if the download within
	                    download_connection doesn't exist in Unique_Download.
	post_send         - Should always be called after sending data obtained from
	                    get_send_buff(). This function erases the send_buff data
	                    in the client_buffer that was sent.
	post_recv         - Should always be called after recv'ing data. Processes
	                    recv'ed data.
	stop_download     - Stops the download associated with hash.
	*/
	static bool add_connection(download_connection & DC);
	static void add_download(locking_shared_ptr<download> & Download);
	static void current_downloads(std::vector<download_status> & info, std::string hash);
	static void erase(const int & socket_FD);
	static void generate_requests();
	static void get_complete(std::vector<locking_shared_ptr<download> > & complete);
	static void get_empty(std::vector<int> & disconnect);
	static bool get_send_buff(const int & socket_FD, const int & max_bytes, std::string & destination);
	static int get_send_pending();
	static void get_timed_out(std::vector<int> & timed_out);
	static bool is_downloading(locking_shared_ptr<download> & Download);
	static bool is_downloading(const std::string & hash);
	static void new_connection(const download_connection & DC);
	static void post_send(const int & socket_FD, const int & n_bytes);
	static void post_recv(const int & socket_FD, char * buff, const int & n_bytes);
	static void stop_download(const std::string & hash);

private:
	//this ctor terminates program because it should not be used
	client_buffer();

	//only the client_buffer can instantiate itself
	client_buffer(const int & socket_in, const std::string & IP_in);

	//locks all public static functions
	static boost::mutex Mutex;

	//all currently running downloads
	static std::set<locking_shared_ptr<download> > Unique_Download;

	//socket mapped to client_buffer
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
	//all downloads that this client_buffer is serving
	std::list<locking_shared_ptr<download> > Download;
	//last download a request was gotten from
	std::list<locking_shared_ptr<download> >::iterator Download_iter;

	std::string IP;   //IP that corresponds to socket
	int socket;

	//used for timeout
	time_t last_seen;

	/*
	This is the dynamic maximum pipeline size.
	global::PIPELINE_SIZE

	When generate_request() is called there are the following cases.
	case: Pipeline.size() == 0 && max_pipeline_size != global::PIPELINE_SIZE
		++max_pipeline_size
	case: Pipeline.size() == 1
		//just right!
	case: Pipeline.size() > 1 && max_pipeline_size != 1
		--max_pipeline_size;
	*/
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
	recv_buff_append   - appends bytes to recv_buff
	empty              - return true if client_buffer contains no downloads
	post_recv_actions  - does actions which may need to be done after a recv
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
	void prepare_request();
	void recv_buff_process();
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

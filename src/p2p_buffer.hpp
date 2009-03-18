//THREADSAFE
#ifndef H_P2P_BUFFER
#define H_P2P_BUFFER

//boost
#include <boost/thread.hpp>
#include <boost/utility.hpp>

//custom
#include "database.hpp"
#include "download.hpp"
#include "download_status.hpp"
#include "encryption.hpp"
#include "http.hpp"
#include "protocol.hpp"
#include "settings.hpp"
#include "slot.hpp"
#include "slot_file.hpp"
#include "slot_hash_tree.hpp"

//include
#include "convert.hpp"

//std
#include <ctime>
#include <deque>
#include <iostream>
#include <list>
#include <map>
#include <set>

class p2p_buffer : private boost::noncopyable
{
public:
	~p2p_buffer();

	//this is called in p2p dtor to clean up this singleton
	static void destroy_all();

	/* Functions for managing connections.
	add_connection:
		Adds a p2p_buffer for a specified socket. Should be called whenever a
		socket is created. This may be called multiple times for the same socket
		and a p2p_buffer will only be created on the first call. The initiator
		parameter controls whether or not we start the key negotiation. If we
		initiate the connection then initiator should equal true.
	erase:
		Erases p2p_buffer associated with socket_FD. This should be called
		whenever a socket is disconnected.
	get_empty:
		Removes all empty p2p_buffers and populates disconnect vector with
		sockets which need to be disconnected.
	get_send_buff:
		Copies max_bytes from the p2p_buffer send_buff in to the destination
		buff. True is returned if destination is not empty after function completes.
		Note 1: max_bytes should be as low as possible to avoid unnecessary copying.
		Note 2: Once a send is done, call post_send to erase the bytes sent from the
		p2p_buffer send_buff.
	get_send_pending:
		Returns counter that indicates how many sockets have bytes to send. Used by
		client to check if write_FDS set should be used.
	get_timed_out:
		Removes p2p_buffers that timed out and populates timed_out vector with
		sockets which should be disconnected.
	is_connected:
		Returns true if there is a connection to the specified IP.
	new_connection:
		Adds download to p2p_buffer. Creates new p2p_buffer if necessary.
		Silently fails if the download within download_connection doesn't exist in
		Unique_Download.
	post_send:
		Should always be called after sending data obtained from get_send_buff().
		This function erases the send_buff data in the p2p_buffer that was sent.
		If returns true then the socket should be disconnected.
	post_recv:
		Should always be called after recv'ing data. Processes recv'ed data.
	process:
		Gives the p2p_buffer time to do necessary processing. Should be called
		before the socket servicing loop.
	*/
	static void add_connection(const int & socket_FD, const std::string & IP, const bool & initiator);
	static void erase(const int & socket_FD);
	static bool get_send_buff(const int & socket_FD, const int & max_bytes, std::string & destination);
	static int get_send_pending();
	static bool is_connected(const std::string & IP);
	static bool post_send(const int & socket_FD, const int & n_bytes);
	static void post_recv(const int & socket_FD, char * buff, const int & n_bytes);
	static void process();

	/* Functions for managing downloads.
	add_download:
		Adds download to unique download set. This is needed because it's possible
		that a download can be started with no connected servers.
	add_download_connection:
		Starts downloading from a download specified by download_connection.
	current_downloads:
		Populates the info vector with download status information. If hash is
		specified then only information from the download which corresponds to hash
		is added to the vector.
		Note: the info vector is cleared before it's populated
	get_complete:
		Populates complete vector with complete downloads. Removes complete
		downloads from all p2p_buffers and Unique_Download.
	is_downloading:
		Returns true if download specified by pointer or hash is running.
	pause_download:
		Stops download from making further requests. Does not disconnect servers,
		but servers may be disconnected if no other downloads are active with them.
	remove_download:
		Removes the download associated with hash.
	remove_download_connection:
		Disassociate a server with a download.
	*/
	static void add_download(boost::shared_ptr<download> & Download);
	static void add_download_connection(download_connection & DC);
	static void current_downloads(std::vector<download_status> & info, std::string hash);
	static void get_complete_downloads(std::vector<boost::shared_ptr<download> > & complete);
	static bool is_downloading(boost::shared_ptr<download> & Download);
	static bool is_downloading(const std::string & hash);
	static void pause_download(const std::string & hash);
	static void remove_download(const std::string & hash);
	static void remove_download_connection(const std::string & hash, const std::string & IP);

	/*Functions for managing uploads.
	current_uploads:
		Populates the info vector with upload_info for all running uploads.
	*/
	static void current_uploads(std::vector<upload_info> & info);

private:
	//initializer for static members
	static boost::once_flag once_flag;
	static void init()
	{
		_Mutex = new boost::mutex();
		_P2P_Buffer = new std::map<int, boost::shared_ptr<p2p_buffer> >();
		_send_pending = new int(0);
		_Unique_Download = new std::set<boost::shared_ptr<download> >();
	}
	//do not use these directly, use the accessor functions
	static boost::mutex * _Mutex;
	static std::map<int, boost::shared_ptr<p2p_buffer> > * _P2P_Buffer;
	static std::set<boost::shared_ptr<download> > * _Unique_Download;
	static int * _send_pending;

	//accessor functions
	static boost::mutex & Mutex()
	{
		boost::call_once(init, once_flag);
		return *_Mutex;
	}
	static std::map<int, boost::shared_ptr<p2p_buffer> > & P2P_Buffer()
	{
		boost::call_once(init, once_flag);
		return *_P2P_Buffer;
	}
	static int & send_pending()
	{
		boost::call_once(init, once_flag);
		return *_send_pending;
	}
	static std::set<boost::shared_ptr<download> > & Unique_Download()
	{
		boost::call_once(init, once_flag);
		return *_Unique_Download;
	}

	p2p_buffer()
	{
		LOGGER << "improperly constructed p2p_buffer";
		exit(1);
	}

	/*
	The initiator parameter controls whether we start key negotiation or not. If
	we initiate the connection we need to send the prime and our g^x % p.
	*/
	p2p_buffer(
		const int & socket_in,
		const std::string & IP_in,
		const bool & initiator_in
	);

	/************************* BEGIN GENERAL ***********************************/

	//what socket/IP this p2p_buffer is for
	int socket;
	std::string IP;

	//true if we initiated the connection, used for key exchange
	bool initiator;

	//main send/recv buffers
	std::string recv_buff;
	std::string send_buff;

	/*
	This placeholder indicates how many bytes in the recv_buff have already been
	seen in previous calls to post_recv(). This is used to update the
	speed_calculators in the downloads.
	*/
	unsigned bytes_seen;

	/*
	This is set to true when key exchange happening. While true the p2p_buffer
	will not get requests from downloads.
	*/
	bool key_exchange;

	encryption Encryption;
	/************************* END GENERAL *************************************/

	/************************* BEGIN DOWNLOAD RELATED **************************/
	/*
	The number of slots in use by the downloads contained within this p2p_buffer.
	This is passed to downloads by reference so they may modify it. Checks exist
	in the p2p_buffer such that if this value goes negative or above 255 the
	program will be terminated.
	*/
	int download_slots_used;

	/*
	The Download container is effectively a ring. The rotate_downloads() function
	will move Download_iter through it in a circular fashion.
	*/
	//all downloads that this p2p_buffer is serving
	std::list<boost::shared_ptr<download> > Download;
	//last download a request was gotten from
	std::list<boost::shared_ptr<download> >::iterator Download_iter;

	/*
	This is the dynamic maximum pipeline size.
	settings::PIPELINE_SIZE

	When generate_request() is called there are the following cases.
	case: Pipeline.size() == 0 && max_pipeline_size != settings::PIPELINE_SIZE
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
		boost::shared_ptr<download> Download;
	};
	/*
	Past requests are stored here. The front of this queue will contain the oldest
	requests. When a response comes in it will correspond to the pending_response
	in the front of this queue.
	*/
	std::deque<pending_response> Pipeline;

	/*
	register_download:
		Register a download with this p2p_buffer. This makes the file download
		from this server.
	rotate_downloads:
		Moves Download_iter through Download in a circular fashion. Returns true
		if looped back to beginning
	terminate_download:
		Removes the download from this p2p_buffer which corresponds to hash. Or
		removal is also possible by supplying boost::shared_ptr to download.
	*/
	void prepare_download_requests();
	void register_download(boost::shared_ptr<download> new_download);
	bool rotate_downloads();
	void terminate_download(const std::string & hash);
	void terminate_download(boost::shared_ptr<download> term_DL);
	/************************* END DOWNLOAD RELATED ****************************/

	/************************* BEGIN UPLOAD RELATED ****************************/
	/*
	The array location is the slot_ID. The string holds the path to the file the
	slot is for. If the string is empty the slot is not yet taken.
	*/
	slot * Upload_Slot[256];

	void close_slot(const std::string & request);
	bool find_empty_slot(const std::string & root_hash, int & slot_num);
	void request_slot_file(const std::string & request, std::string & send);
	void request_slot_hash(const std::string & request, std::string & send);
	void send_block(const std::string & request, std::string & send);
	void uploads(std::vector<upload_info> & UI);
	/************************* END UPLOAD RELATED ******************************/

	//used when http response sent to localhost to disconnect after send
	bool http_response_sent;

	/*General Functions
	protocol_localhost:
		Does all processing of recv_buff for localhost.
	protocl_key_exchange:
		Handles key exchange for both the host connected to and the host that is
		connecting.
	protocol_response:
		Handles responses to requests. Returns true if a response was parsed, false
		if not enough bytes in buffer for a full response. This function processes
		only one response when called so it should be called until it returns false,
		or until the recv_buffer is empty.
	protocol_request:
		Handles requests. Returns true if a request was parsed (and response contains
		data that needs to be sent). Returns false if not enough bytes in buffer for
		a full request.
	recv_buff_process:
		Appends data to recv_buff and processes it.
	recv_buff_read_block:
		Reads one block from recv_buff. Returns true if block read, or false if
		block incomplete.
	*/
	void protocol_localhost(char * buff, const int & n_bytes);
	void protocol_key_exchange(char * buff, const int & n_bytes);
	bool protocol_response();
	bool protocol_request(std::string & response);
	void recv_buff_process(char * buff, const int & n_bytes);

	database::table::blacklist DB_Blacklist;
	database::table::download DB_Download;
	database::table::share DB_Share;
	hash_tree Hash_Tree;
	http HTTP;
};
#endif

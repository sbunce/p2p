//THREADSAFE
#ifndef H_SERVER_BUFFER
#define H_SERVER_BUFFER

//boost
#include <boost/thread.hpp>
#include <boost/utility.hpp>

//custom
#include "convert.hpp"
#include "database.hpp"
#include "encryption.hpp"
#include "global.hpp"
#include "hash_tree.hpp"
#include "http.hpp"
#include "slot.hpp"
#include "slot_file.hpp"
#include "slot_hash_tree.hpp"
#include "speed_calculator.hpp"
#include "upload_info.hpp"

//std
#include <map>
#include <vector>

class server_buffer : private boost::noncopyable
{
public:
	~server_buffer();

	/*
	current_uploads  - Populates info vector with current upload info.
	                   Note: info cleared before it's populated
	erase            - Removes the server buffer associated socket_FD.
	                   Note: This is called whenever a socket disconnected.
	get_send_buff    - DO DOCUMENTATION
	get_send_pending - Returns counter that indicates how many sockets have bytes
	                   to send. Used by server to check if write_FDS set should be
	                   used.
	new_connection   - creates new server_buffer for socket_FD
	post_send        - DO DOCUMENTATION
	process          - DO DOCUMENTATION
	*/
	static void current_uploads(std::vector<upload_info> & info);
	static void erase(const int & socket_FD);
	static bool get_send_buff(const int & socket_FD, const int & max_bytes, std::string & destination);
	static int get_send_pending();
	static void new_connection(const int & socket_FD, const std::string & IP);
	static bool post_send(const int & socket_FD, const int & n_bytes);
	static void post_recv(const int & socket_FD, char * recv_buff, const int & n_bytes);

private:
	//this ctor terminates program because it should not be used
	server_buffer();

	//only the server_buffer can instantiate itself
	server_buffer(const int & socket_FD_in, const std::string & IP_in);

	//mutex for all static public functions
	static boost::mutex Mutex;

	//socket mapped to server_buffer
	static std::map<int, boost::shared_ptr<server_buffer> > Server_Buffer;

	/*
	When this is zero there are no pending requests to send. This exists for the
	purpose of having an alternate select() call that doesn't involve write_FDS
	because using write_FDS hogs CPU. When the value referenced by this equals 0
	then there are no sends pending and write_FDS doesn't need to be used. These
	are given to the client_buffer elements so they can increment it.
	*/
	static int send_pending;

	/*
	Certain responses require the server be disconnected upon message send. When
	this is true the server is disconnected when the send_buff is empty.
	*/
	bool disconnect_on_empty;

	//main send/recv buffers
	std::string recv_buff;
	std::string send_buff;

	//socket and IP this server_buffer instantiation is for
	int socket_FD;
	std::string IP;

	/*
	The array location is the slot_ID. The string holds the path to the file the
	slot is for. If the string is empty the slot is not yet taken.
	*/
	slot * Slot[256];

	//temp buffers used by process()
	std::string process_send;
	std::string process_request;

	/*
	close_slot        - frees slot memory in Slot, sets pointer to NULL to indicate empty
	find_empty_slot   - locates and empty slot
	                    returns true and sets slot_num if empty slot found
	                    returns false if all slots full (this should trigger blacklist)
	recv_buff_process - decrypt incoming bytes, break apart individual messages, send
	                    messages to processing functions
	request_slot_hash - prepares response to a request for a hash tree slot
	request_slot_file - prepares response to a request for a file slot
	send_block        - prepares response for a hash block or file block request
	uploads           - returns information about files currently uploading
	*/
	void close_slot(const std::string & request);
	bool find_empty_slot(const std::string & root_hash, int & slot_num);
	void recv_buff_process(char * buff, const int & n_bytes);
	void request_slot_hash(const std::string & request, std::string & send);
	void request_slot_file(const std::string & request, std::string & send);
	void send_block(const std::string & request, std::string & send);
	void uploads(std::vector<upload_info> & UI);

	//encryption stuff
	bool exchange_key;               //true when key exchange happening
	std::string prime_remote_result; //holds prime and incoming result for key exchange
	encryption Encryption;           //object to do stream cyphers

	database::table::blacklist DB_Blacklist;
	database::table::download DB_Download;
	database::table::share DB_Share;
	hash_tree Hash_Tree;
	http HTTP;
};
#endif

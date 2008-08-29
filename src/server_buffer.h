#ifndef H_SERVER_BUFFER
#define H_SERVER_BUFFER

//boost
#include <boost/thread/mutex.hpp>

//custom
#include "convert.h"
#include "client_server_bridge.h"
#include "DB_blacklist.h"
#include "DB_share.h"
#include "encryption.h"
#include "global.h"
#include "speed_calculator.h"
#include "upload_info.h"

//std
#include <map>
#include <vector>

class server_buffer
{
public:
	/*
	WARNING: do not use this ctor it will terminate the program.
	*/
	server_buffer();

	~server_buffer();

	/*
	Populates the info vector with download information for all downloads running
	in all client_buffers. The vector passed in is cleared before download info is
	added.
	*/
	static void current_uploads(std::vector<upload_info> & info)
	{
		boost::mutex::scoped_lock lock(Mutex);
		std::map<int, server_buffer *>::iterator iter_cur, iter_end;
		iter_cur = Server_Buffer.begin();
		iter_end = Server_Buffer.end();
		while(iter_cur != iter_end){
			iter_cur->second->uploads(info);
			++iter_cur;
		}
	}

	/*
	Deletes all downloads in server_buffer and deletes all server_buffer instantiations.
	This is used when server is being destroyed.
	*/
	static void destroy()
	{
		while(!Server_Buffer.empty()){
			delete Server_Buffer.begin()->second;
			Server_Buffer.erase(Server_Buffer.begin());
		}
	}

	/*
	Erases an element from Server_Buffer associated with socket_FD. This should be
	called whenever a socket is disconnected.
	*/
	static void erase(const int & socket_FD)
	{
		boost::mutex::scoped_lock lock(Mutex);
		std::map<int, server_buffer *>::iterator iter = Server_Buffer.find(socket_FD);
		if(iter != Server_Buffer.end()){
			if(!iter->second->send_buff.empty()){
				//send_buff contains data, decrement send_pending
				--send_pending;
			}
			delete iter->second;
			Server_Buffer.erase(iter);
		}
	}

	/*
	Returns the server_buffer send_buff that corresponds to socket_FD.
	WARNING: violates encapsulation, NOT thread safe to call this from multiple threads
	*/
	static std::string & get_send_buff(const int & socket_FD)
	{
		boost::mutex::scoped_lock lock(Mutex);
		std::map<int, server_buffer *>::iterator iter = Server_Buffer.find(socket_FD);
		assert(iter != Server_Buffer.end());
		return iter->second->send_buff;
	}

	/*
	Returns the counter that indicates whether a send needs to be done. If it is
	zero no send needs to be done. Every +1 above zero it is indicates a send_buff
	has something in it.
	*/
	static const volatile int & get_send_pending()
	{
		return send_pending;
	}

	static void new_connection(const int & socket_FD, const std::string & IP)
	{
		boost::mutex::scoped_lock lock(Mutex);
		Server_Buffer.insert(std::make_pair(socket_FD, new server_buffer(socket_FD, IP)));
	}

	/*
	Should be called after sending data. This function evaluates whether or not the
	send_buff was emptied. If it was then it decrements send_pending.
	*/
	static void post_send(const int & socket_FD)
	{
		boost::mutex::scoped_lock lock(Mutex);
		std::map<int, server_buffer *>::iterator iter = Server_Buffer.find(socket_FD);
		assert(iter != Server_Buffer.end());

		if(iter->second->send_buff.empty()){
			--send_pending;
		}
	}

	/*
	Stores recv'd bytes in the appropriate server_buffer and processes the buffer.
	*/
	static void process(const int & socket_FD, char * recv_buff, const int & n_bytes)
	{
		boost::mutex::scoped_lock lock(Mutex);
		std::map<int, server_buffer *>::iterator iter = Server_Buffer.find(socket_FD);
		assert(iter != Server_Buffer.end());
		server_buffer * SB = iter->second;
		bool initial_empty = SB->send_buff.size() == 0;
		SB->process(recv_buff, n_bytes);
		if(initial_empty && SB->send_buff.size() != 0){
			//buffer went from empty to non-empty, signal that a send needs to be done
			++send_pending;
		}
	}

private:
	//only the server_buffer can instantiate itself
	server_buffer(const int & socket_FD_in, const std::string & IP_in);

	static boost::mutex Mutex;                           //mutex for any access to a download
	static std::map<int, server_buffer *> Server_Buffer; //socket mapped to server_buffer

	/*
	When this is zero there are no pending requests to send. This exists for the
	purpose of having an alternate select() call that doesn't involve write_FDS
	because using write_FDS hogs CPU. When the value referenced by this equals 0
	then there are no sends pending and write_FDS doesn't need to be used. These
	are given to the client_buffer elements so they can increment it.
	*/
	static volatile int send_pending;

	//main send/recv buffers
	std::string recv_buff;
	std::string send_buff;

	//socket and IP this server_buffer instantiation is for
	int socket_FD;
	std::string IP;

	enum slot_type{
		SLOT_HASH_TREE,
		SLOT_FILE
	};

	class slot_element
	{
	public:
		slot_element(
			const std::string & hash_in,
			const boost::uint64_t & size_in,
			const std::string & path_in,
			const slot_type & Slot_Type_in
		):
			hash(hash_in),
			size(size_in),
			path(path_in),
			Slot_Type(Slot_Type_in),
			percent_complete(0)
		{
			assert(path.find_last_of("/") != std::string::npos);
			name = path.substr(path.find_last_of("/")+1);
		}

		slot_type Slot_Type;  //what type of slot this is
		std::string hash;
		boost::uint64_t size;
		std::string path;     //path of file associated with slot
		std::string name;     //name of file
		int percent_complete; //what % client has downloaded

		speed_calculator Speed_Calculator;
	};

	/*
	The array location is the slot_ID. The string holds the path to the file the
	slot is for. If the string is empty the slot is not yet taken.
	*/
	slot_element * Slot[256];

	//buffer for reading file blocks from the HDD
	char send_block_buff[global::FILE_BLOCK_SIZE];

	//temp buffers used by process()
	std::string process_send;
	std::string process_request;

	/*
	create_slot_file             - create a download slot for a file
	                               returns true if slot successfully created, else false
	current_uploads              - adds upload information to the info vector
	get_slot_element             - returns slot element associated with slot ID, returns NULL if bad slot
	update_slot_percent_complete - updates the percentage complete an upload is
	update_slot_speed            - updates speed for the upload the corresponds to the slot_ID
	*/
	void close_slot(const std::string & request);
	bool create_slot(char & slot_ID, const std::string & hash, const boost::uint64_t & size, const std::string & path, const slot_type ST);
	void uploads(std::vector<upload_info> & info);
	slot_element * get_slot_element(const char & slot_ID);
	void process(char * buff, const int & n_bytes);
	void request_slot_hash(const std::string & request, std::string & send);
	void request_slot_file(const std::string & request, std::string & send);
	void make_slot_file(const std::string & root_hash_hex, const boost::uint64_t & size, const std::string & path, std::string & send);
	void send_block(const std::string & request, std::string & send);
	void update_slot_percent_complete(char slot_ID, const boost::uint64_t & block_number);
	void update_slot_speed(char slot_ID, unsigned int bytes);

	bool exchange_key;               //true when key exchange happening
	std::string prime_remote_result; //holds prime and incoming result for key exchange
	encryption Encryption;

	DB_share DB_Share;
};
#endif

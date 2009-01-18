#ifndef H_SERVER_BUFFER
#define H_SERVER_BUFFER

//boost
#include <boost/thread/mutex.hpp>

//custom
#include "convert.h"
#include "DB_blacklist.h"
#include "DB_download.h"
#include "DB_share.h"
#include "encryption.h"
#include "global.h"
#include "hash_tree.h"
#include "http.h"
#include "slot.h"
#include "slot_file.h"
#include "slot_hash_tree.h"
#include "speed_calculator.h"
#include "upload_info.h"

//std
#include <map>
#include <vector>

class server_buffer
{
public:
	/*
	WARNING: do not use this ctor, it will terminate the program.
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
	static int get_send_pending()
	{
		boost::mutex::scoped_lock lock(Mutex);
		return send_pending;
	}

	static void new_connection(const int & socket_FD, const std::string & IP)
	{
		boost::mutex::scoped_lock lock(Mutex);
		Server_Buffer.insert(std::make_pair(socket_FD, new server_buffer(socket_FD, IP)));
	}

	/*
	Should be called after sending data. This function evaluates whether or not the
	send_buff was emptied. If it was then it decrements send_pending. Returns false
	to indicate the server should be disconnected.
	*/
	static bool post_send(const int & socket_FD)
	{
		boost::mutex::scoped_lock lock(Mutex);
		std::map<int, server_buffer *>::iterator iter = Server_Buffer.find(socket_FD);
		assert(iter != Server_Buffer.end());
		if(iter->second->send_buff.empty()){
			--send_pending;
		}

		if(iter->second->disconnect_on_empty && iter->second->send_buff.empty()){
			return false;
		}else{
			return true;
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
	process           - decrypt incoming bytes, break apart individual messages, send
	                    messages to processing functions
	request_slot_hash - prepares response to a request for a hash tree slot
	request_slot_file - prepares response to a request for a file slot
	send_block        - prepares response for a hash block or file block request
	uploads           - returns information about files currently uploading
	*/
	void close_slot(const std::string & request);
	bool find_empty_slot(const std::string & root_hash, int & slot_num);
	void process(char * buff, const int & n_bytes);
	void request_slot_hash(const std::string & request, std::string & send);
	void request_slot_file(const std::string & request, std::string & send);
	void send_block(const std::string & request, std::string & send);
	void uploads(std::vector<upload_info> & UI);

	//encryption stuff
	bool exchange_key;               //true when key exchange happening
	std::string prime_remote_result; //holds prime and incoming result for key exchange
	encryption Encryption;           //object to do stream cyphers

	DB_blacklist DB_Blacklist;
	DB_download DB_Download;
	DB_share DB_Share;
	hash_tree Hash_Tree;
	http HTTP;
};
#endif

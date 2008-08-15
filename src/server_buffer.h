#ifndef H_SERVER_BUFFER
#define H_SERVER_BUFFER

//boost
#include <boost/thread/mutex.hpp>

//custom
#include "convert.h"
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
			iter_cur->second->current_uploads_priv(info);
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
		assert(iter != Server_Buffer.end());
		if(!iter->second->send_buff.empty()){
			//send_buff contains data, decrement send_pending
			--send_pending;
		}
		delete iter->second;
		Server_Buffer.erase(iter);
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

		if(SB->exchange_key){
			SB->prime_remote_result.append(recv_buff, n_bytes);
			if(SB->prime_remote_result.size() == global::DH_KEY_SIZE*2){
				SB->Encryption.set_prime(SB->prime_remote_result.substr(0,global::DH_KEY_SIZE));
				SB->Encryption.set_remote_result(SB->prime_remote_result.substr(global::DH_KEY_SIZE,global::DH_KEY_SIZE));
				SB->send_buff += SB->Encryption.get_local_result();
				SB->prime_remote_result.clear();
				SB->exchange_key = false;
			}

			if(SB->prime_remote_result.size() > global::DH_KEY_SIZE*2){
				logger::debug(LOGGER_P1,"abusive client ", SB->IP, " failed key negotation, too many bytes");
				DB_blacklist::add(SB->IP);
			}

			if(initial_empty && SB->send_buff.size() != 0){
				//buffer went from empty to non-empty, signal that a send needs to be done
				++send_pending;
			}

			return;
		}

		//decrypt received bytes
		SB->Encryption.crypt_recv(recv_buff, n_bytes);
		SB->recv_buff.append(recv_buff, n_bytes);

		//disconnect clients that have pipelined more than is allowed
		if(SB->recv_buff.size() > global::S_MAX_SIZE*global::PIPELINE_SIZE){
			logger::debug(LOGGER_P1,"server ",SB->IP," over pipelined");
			DB_blacklist::add(SB->IP);
			return;
		}

		//slice off one command at a time and process
		std::string send; //data to send
		while(SB->recv_buff.size()){
			send.clear();
			if(SB->recv_buff[0] == global::P_CLOSE_SLOT && SB->recv_buff.size() >= global::P_CLOSE_SLOT_SIZE){
				SB->close_slot(SB);
				SB->recv_buff.erase(0, global::P_CLOSE_SLOT_SIZE);
			}else if(SB->recv_buff[0] == global::P_REQUEST_SLOT_HASH && SB->recv_buff.size() >= global::P_REQUEST_SLOT_HASH_SIZE){
				SB->request_slot_hash(SB, send);
				SB->recv_buff.erase(0, global::P_REQUEST_SLOT_HASH_SIZE);
			}else if(SB->recv_buff[0] == global::P_REQUEST_SLOT_FILE && SB->recv_buff.size() >= global::P_REQUEST_SLOT_FILE_SIZE){
				SB->request_slot_file(SB, send);
				SB->recv_buff.erase(0, global::P_REQUEST_SLOT_FILE_SIZE);
			}else if(SB->recv_buff[0] == global::P_SEND_BLOCK && SB->recv_buff.size() >= global::P_SEND_BLOCK_SIZE){
				SB->send_block(SB, send);
				SB->recv_buff.erase(0, global::P_SEND_BLOCK_SIZE);
			}else{
				break;
			}

			SB->Encryption.crypt_send(send);
			SB->send_buff += send;
		}

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

	std::string recv_buff; //buffer for partial recvs
	std::string send_buff; //buffer for partial sends

	int socket_FD;
	std::string IP;

	class slot_element
	{
	public:
		slot_element(
			const std::string & hash_in,
			const boost::uint64_t & size_in,
			const std::string & path_in
		):
			hash(hash_in),
			size(size_in),
			path(path_in),
			percent_complete(0)
		{
			assert(path.find_last_of("/") != std::string::npos);
			name = path.substr(path.find_last_of("/")+1);
		}

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

	/*
	These functions correspond to the protocol command names. Documentation for
	what they do can be found in the protocol documentation.
	*/
	void close_slot(server_buffer * SB);
	void request_slot_hash(server_buffer * SB, std::string & send);
	void request_slot_file(server_buffer * SB, std::string & send);
	void send_block(server_buffer * SB, std::string & send);

	/*
	close_slot                   - close a slot opened with create_slot()
	create_slot_file             - create a download slot for a file
	                               returns true if slot successfully created, else false
	current_uploads              - adds upload information to the info vector
	path                         - sets path to uploading file path, returns false if slot invalid, else true
	update_slot_percent_complete - updates the percentage complete an upload is
	update_slot_speed            - updates speed for the upload the corresponds to the slot_ID
	*/
	void close_slot(char slot_ID);
	bool create_slot(char & slot_ID, const std::string & hash, const boost::uint64_t & size, const std::string & path);
	void current_uploads_priv(std::vector<upload_info> & info);
	bool path(char slot_ID, std::string & path);
	void update_slot_percent_complete(char slot_ID, const boost::uint64_t & block_number);
	void update_slot_speed(char slot_ID, unsigned int bytes);

	DB_share DB_Share;

	bool exchange_key;               //true when key exchange happening
	std::string prime_remote_result; //holds prime and incoming result for key exchange
	encryption Encryption;
};
#endif

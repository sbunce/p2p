#include "server_protocol.h"

server_protocol::server_protocol()
{
	#ifdef CORRUPT_BLOCKS
	srand(time(NULL));
	#endif
}

void server_protocol::process(server_buffer * SB, char * recv_buff, const int & n_bytes)
{
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
		return;
	}

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
			close_slot(SB);
			SB->recv_buff.erase(0, global::P_CLOSE_SLOT_SIZE);
		}else if(SB->recv_buff[0] == global::P_REQUEST_SLOT_HASH && SB->recv_buff.size() >= global::P_REQUEST_SLOT_HASH_SIZE){
			request_slot_hash(SB, send);
			SB->recv_buff.erase(0, global::P_REQUEST_SLOT_HASH_SIZE);
		}else if(SB->recv_buff[0] == global::P_REQUEST_SLOT_FILE && SB->recv_buff.size() >= global::P_REQUEST_SLOT_FILE_SIZE){
			request_slot_file(SB, send);
			SB->recv_buff.erase(0, global::P_REQUEST_SLOT_FILE_SIZE);
		}else if(SB->recv_buff[0] == global::P_SEND_BLOCK && SB->recv_buff.size() >= global::P_SEND_BLOCK_SIZE){
			send_block(SB, send);
			SB->recv_buff.erase(0, global::P_SEND_BLOCK_SIZE);
		}else{
			break;
		}

		SB->Encryption.crypt_send(send);
		SB->send_buff += send;
	}
}

void server_protocol::close_slot(server_buffer * SB)
{
	logger::debug(LOGGER_P1,"closing slot ",(int)(unsigned char)SB->recv_buff[1], " for ",SB->IP);
	SB->close_slot(SB->recv_buff[1]);
}

void server_protocol::request_slot_hash(server_buffer * SB, std::string & send)
{
	std::string path = global::HASH_DIRECTORY+convert::binary_to_hex(SB->recv_buff.substr(1,20));
	std::fstream fin(path.c_str(), std::ios::in);
	if(fin.is_open()){
		//hash tree exists, make slot
		fin.seekg(0, std::ios::end);
		boost::uint64_t size = fin.tellg();
		char slot_ID;
		if(SB->create_slot(slot_ID, convert::binary_to_hex(SB->recv_buff.substr(1,20)), size, path)){
			logger::debug(LOGGER_P1,"granting hash slot ",(int)(unsigned char)slot_ID, " to ",SB->IP);
			send += global::P_SLOT_ID;
			send += slot_ID;
		}else{
			//all slots used up
			logger::debug(LOGGER_P1,"all slots used up, sending error to ",SB->IP);
			send += global::P_ERROR;
		}
	}else{
		//hash tree doesn't exist
		logger::debug(LOGGER_P1,SB->IP," requested hash that doesn't exist");
		send += global::P_ERROR;
	}
}

void server_protocol::request_slot_file(server_buffer * SB, std::string & send)
{
	boost::uint64_t size;
	std::string path;
	if(DB_Share.lookup_hash(convert::binary_to_hex(SB->recv_buff.substr(1,20)), size, path)){
		//hash found in share, create slot
		char slot_ID;
		if(SB->create_slot(slot_ID, convert::binary_to_hex(SB->recv_buff.substr(1,20)), size, path)){
			logger::debug(LOGGER_P1,"granting file slot ",(int)(unsigned char)slot_ID, " to ",SB->IP);
			send += global::P_SLOT_ID;
			send += slot_ID;
			SB->update_slot_speed(slot_ID, global::P_REQUEST_SLOT_FILE_SIZE);
		}else{
			//all slots used up
			logger::debug(LOGGER_P1,"all slots used up, sending error to ",SB->IP);
			send += global::P_ERROR;
		}
	}else{
		//hash not found in share, send error
		logger::debug(LOGGER_P1,SB->IP," requested hash that doesn't exist");
		send += global::P_ERROR;
	}
}

void server_protocol::send_block(server_buffer * SB, std::string & send)
{
	std::string path;
	if(SB->path(SB->recv_buff[1], path)){
		boost::uint64_t block_number = convert::decode<boost::uint64_t>(SB->recv_buff.substr(2, 8));
		SB->update_slot_percent_complete(SB->recv_buff[1], block_number);
		send += global::P_BLOCK;
		std::ifstream fin(path.c_str());
		if(fin.is_open()){
			//seek to the file_block the client wants (-1 for command space)
			fin.seekg(block_number * global::FILE_BLOCK_SIZE);
			fin.read(send_block_buff, global::FILE_BLOCK_SIZE);

			#ifdef CORRUPT_BLOCKS
			if(rand() % 100 == 0){
				send_block_buff[0] = (char)0;
			}
			#endif

			send.append(send_block_buff, fin.gcount());
			SB->update_slot_speed(SB->recv_buff[1], fin.gcount());
		}
	}else{
		logger::debug(LOGGER_P1,SB->IP," sent a invalid slot ID");
	}
}

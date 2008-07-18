#include "server_protocol.h"

server_protocol::server_protocol()
{
	#ifdef CORRUPT_BLOCKS
	srand(time(NULL));
	#endif
}

void server_protocol::process(server_buffer * SB)
{
	//slice off one command at a time and process
	while(SB->recv_buff.size()){
		if(SB->recv_buff[0] == global::P_CLOSE_SLOT && SB->recv_buff.size() >= global::P_CLOSE_SLOT_SIZE){
			close_slot(SB);
			SB->recv_buff.erase(0, global::P_CLOSE_SLOT_SIZE);
		}else if(SB->recv_buff[0] == global::P_REQUEST_SLOT_HASH && SB->recv_buff.size() >= global::P_REQUEST_SLOT_HASH_SIZE){
			request_slot_hash(SB);
			SB->recv_buff.erase(0, global::P_REQUEST_SLOT_HASH_SIZE);
		}else if(SB->recv_buff[0] == global::P_REQUEST_SLOT_FILE && SB->recv_buff.size() >= global::P_REQUEST_SLOT_FILE_SIZE){
			request_slot_file(SB);
			SB->recv_buff.erase(0, global::P_REQUEST_SLOT_FILE_SIZE);
		}else if(SB->recv_buff[0] == global::P_SEND_BLOCK && SB->recv_buff.size() >= global::P_SEND_BLOCK_SIZE){
			send_block(SB);
			SB->recv_buff.erase(0, global::P_SEND_BLOCK_SIZE);
		}else if(SB->recv_buff.length() >= 3 && strncmp(SB->recv_buff.c_str(), "GET", 3) == 0){
			test(SB);
			SB->recv_buff.erase(0,1);
		}else{
			break;
		}
	}
}

void server_protocol::close_slot(server_buffer * SB)
{
	logger::debug(LOGGER_P1,"closing slot ",(int)(unsigned char)SB->recv_buff[1], " for ",SB->IP);
	SB->close_slot(SB->recv_buff[1]);
}

void server_protocol::request_slot_hash(server_buffer * SB)
{
	std::string path = global::HASH_DIRECTORY+convert::binary_to_hex(SB->recv_buff.substr(1,20));
	std::fstream fin(path.c_str(), std::ios::in);
	if(fin.is_open()){
		//hash tree exists, make slot
		fin.seekg(0, std::ios::end);
		uint64_t size = fin.tellg();
		char slot_ID;
		if(SB->create_slot(slot_ID, convert::binary_to_hex(SB->recv_buff.substr(1,20)), size, path)){
			logger::debug(LOGGER_P1,"granting hash slot ",(int)(unsigned char)slot_ID, " to ",SB->IP);
			SB->send_buff += global::P_SLOT_ID;
			SB->send_buff += slot_ID;
		}else{
			//all slots used up
			logger::debug(LOGGER_P1,"all slots used up, sending error to ",SB->IP);
			SB->send_buff += global::P_ERROR;
		}
	}else{
		//hash tree doesn't exist
		logger::debug(LOGGER_P1,SB->IP," requested hash that doesn't exist");
		SB->send_buff += global::P_ERROR;
	}
}

void server_protocol::request_slot_file(server_buffer * SB)
{
	uint64_t size;
	std::string path;
	if(DB_Share.lookup_hash(convert::binary_to_hex(SB->recv_buff.substr(1,20)), size, path)){
		//hash found in share, create slot
		char slot_ID;
		if(SB->create_slot(slot_ID, convert::binary_to_hex(SB->recv_buff.substr(1,20)), size, path)){
			logger::debug(LOGGER_P1,"granting file slot ",(int)(unsigned char)slot_ID, " to ",SB->IP);
			SB->send_buff += global::P_SLOT_ID;
			SB->send_buff += slot_ID;
			SB->update_slot_speed(slot_ID, global::P_REQUEST_SLOT_FILE_SIZE);
		}else{
			//all slots used up
			logger::debug(LOGGER_P1,"all slots used up, sending error to ",SB->IP);
			SB->send_buff += global::P_ERROR;
		}
	}else{
		//hash not found in share, send error
		logger::debug(LOGGER_P1,SB->IP," requested hash that doesn't exist");
		SB->send_buff += global::P_ERROR;
	}
}

void server_protocol::send_block(server_buffer * SB)
{
	std::string path;
	if(SB->path(SB->recv_buff[1], path)){
		uint64_t block_number = convert::decode<uint64_t>(SB->recv_buff.substr(2, 8));
		SB->update_slot_percent_complete(SB->recv_buff[1], block_number);
		SB->send_buff += global::P_BLOCK;
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

			SB->send_buff.append(send_block_buff, fin.gcount());
			SB->update_slot_speed(SB->recv_buff[1], fin.gcount());
		}
	}else{
		logger::debug(LOGGER_P1,SB->IP," sent a invalid slot ID");
	}
}

void server_protocol::test(server_buffer * SB)
{
	SB->send_buff += "<html>hi!</html>";
	SB->send_buff += '\0';
}

#include "server_protocol.h"

server_protocol::server_protocol()
{

}

void server_protocol::process(server_buffer * SB)
{
	//slice off one command at a time and process
	while(SB->recv_buff.size()){
		if(SB->recv_buff[0] == global::P_REQUEST_SLOT_FILE && SB->recv_buff.size() >= global::P_REQUEST_SLOT_FILE_SIZE){
			request_slot_file(SB);
			SB->recv_buff.erase(0, global::P_REQUEST_SLOT_FILE_SIZE);
		}else if(SB->recv_buff[0] == global::P_SEND_BLOCK && SB->recv_buff.size() >= global::P_SEND_BLOCK_SIZE){
			send_block(SB);
			SB->recv_buff.erase(0, global::P_SEND_BLOCK_SIZE);
		}else if(SB->recv_buff[0] == global::P_CLOSE_SLOT && SB->recv_buff.size() >= global::P_CLOSE_SLOT_SIZE){
			close_slot(SB);
			SB->recv_buff.erase(0, global::P_CLOSE_SLOT_SIZE);
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

void server_protocol::request_slot_file(server_buffer * SB)
{
	uint64_t size;
	std::string path;
	if(DB_Share.lookup_hash(hex::binary_to_hex(SB->recv_buff.substr(1,20)), size, path)){
		//hash found in share, create slot
		char slot_ID;
		if(SB->create_slot(slot_ID, hex::binary_to_hex(SB->recv_buff.substr(1,20)), size, path)){
			logger::debug(LOGGER_P1,"granting slot ",(int)(unsigned char)slot_ID, " to ",SB->IP);
			//slot available
			SB->send_buff += global::P_SLOT_ID;
			SB->send_buff += slot_ID;
			SB->update_slot_speed(SB->recv_buff[1], global::P_REQUEST_SLOT_FILE_SIZE);
		}else{
			//all slots used up
			SB->send_buff += global::P_ERROR;
			SB->update_slot_speed(SB->recv_buff[1], 1);
		}
	}else{
		//hash not found in share, send error
		SB->send_buff += global::P_ERROR;
		SB->update_slot_speed(SB->recv_buff[1], 1);
	}
}

void server_protocol::send_block(server_buffer * SB)
{
	uint64_t block_number = Convert_uint64.decode(SB->recv_buff.substr(2, 8));
	if(SB->slot_valid(SB->recv_buff[1])){
		SB->update_slot_percent_complete(SB->recv_buff[1], block_number);
		SB->send_buff += global::P_BLOCK;
		std::ifstream fin(SB->path(SB->recv_buff[1]).c_str());
		if(fin.is_open()){
			//seek to the file_block the client wants (-1 for command space)
			fin.seekg(block_number*(global::P_BLOCK_SIZE - 1));
			fin.read(send_block_buff, global::P_BLOCK_SIZE - 1);
			SB->send_buff.append(send_block_buff, fin.gcount());
			fin.close();
			SB->update_slot_speed(SB->recv_buff[1], fin.gcount());
		}
	}
}

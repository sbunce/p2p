#include "server_buffer.h"

server_buffer::server_buffer(
	const int & socket_in,
	const std::string & IP_in
):
	socket(socket_in),
	IP(IP_in)
{
	send_buff.reserve(global::C_MAX_SIZE * global::PIPELINE_SIZE);
	recv_buff.reserve(global::S_MAX_SIZE * global::PIPELINE_SIZE);

	for(int x=0; x<256; ++x){
		Slot[x] = NULL;
	}
}

server_buffer::~server_buffer()
{
	for(int x=0; x<256; ++x){
		if(Slot[x] != NULL){
			delete Slot[x];
		}
	}
}

void server_buffer::close_slot(char slot_ID)
{
	if(Slot[(int)(unsigned char)slot_ID] != NULL){
		delete Slot[(int)(unsigned char)slot_ID];
		Slot[(int)(unsigned char)slot_ID] = NULL;
	}
}

bool server_buffer::create_slot(char & slot_ID, const std::string & hash, const uint64_t & size, const std::string & path)
{
	for(int x=0; x<256; ++x){
		if(Slot[x] == NULL){
			slot_ID = (char)x;
			Slot[x] = new slot_element(hash, size, path);
			return true;
		}
	}
	return false;
}

void server_buffer::current_uploads(std::vector<upload_info> & info)
{
	for(int x=0; x<256; ++x){
		if(Slot[x] != NULL){
			info.push_back(upload_info(
				Slot[x]->hash,
				IP,
				Slot[x]->size,
				Slot[x]->path,
				Slot[x]->Speed_Calculator.speed(),
				Slot[x]->percent_complete
			));
		}
	}
}

std::string & server_buffer::path(char slot_ID)
{
	//path should not be called unless slot first validated with slot_valid()
	assert(Slot[(int)(unsigned char)slot_ID] != NULL);

	return Slot[(int)(unsigned char)slot_ID]->path;
}

bool server_buffer::slot_valid(char slot_ID)
{
	return Slot[(int)(unsigned char)slot_ID] != NULL;
}

void server_buffer::update_slot_percent_complete(char slot_ID, const uint64_t & block_number)
{
	if(Slot[(int)(unsigned char)slot_ID] != NULL){
		if(block_number != 0){
			Slot[(int)(unsigned char)slot_ID]->percent_complete =
				(int)(((double)(block_number * (global::P_BLOCK_SIZE - 1)) / (double)Slot[(int)(unsigned char)slot_ID]->size)*100);
			if(Slot[(int)(unsigned char)slot_ID]->percent_complete > 100){
				Slot[(int)(unsigned char)slot_ID]->percent_complete = 100;
			}
		}
	}
}

void server_buffer::update_slot_speed(char slot_ID, unsigned int bytes)
{
	if(Slot[(int)(unsigned char)slot_ID] != NULL){
		Slot[(int)(unsigned char)slot_ID]->Speed_Calculator.update(bytes);
	}
}



#include "server_buffer.h"

server_buffer::server_buffer(
	const int & socket_FD_in,
	const std::string & IP_in
):
	socket_FD(socket_FD_in),
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
	boost::mutex::scoped_lock lock(Mutex);
	if(Slot[(int)(unsigned char)slot_ID] != NULL){
		delete Slot[(int)(unsigned char)slot_ID];
		Slot[(int)(unsigned char)slot_ID] = NULL;
	}
}

bool server_buffer::create_slot(char & slot_ID, const std::string & hash, const uint64_t & size, const std::string & path)
{
	boost::mutex::scoped_lock lock(Mutex);
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
	boost::mutex::scoped_lock lock(Mutex);
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

bool server_buffer::path(char slot_ID, std::string & path)
{
	boost::mutex::scoped_lock lock(Mutex);
	if(Slot[(int)(unsigned char)slot_ID] != NULL){
		path = Slot[(int)(unsigned char)slot_ID]->path;
		return true;
	}else{
		return false;
	}
}

void server_buffer::update_slot_percent_complete(char slot_ID, const uint64_t & block_number)
{
	boost::mutex::scoped_lock lock(Mutex);
	if(Slot[(int)(unsigned char)slot_ID] != NULL){
		if(block_number != 0){
			Slot[(int)(unsigned char)slot_ID]->percent_complete =
				(int)(((double)(block_number * global::FILE_BLOCK_SIZE) / (double)Slot[(int)(unsigned char)slot_ID]->size)*100);
			if(Slot[(int)(unsigned char)slot_ID]->percent_complete > 100){
				Slot[(int)(unsigned char)slot_ID]->percent_complete = 100;
			}
		}
	}
}

void server_buffer::update_slot_speed(char slot_ID, unsigned int bytes)
{
	boost::mutex::scoped_lock lock(Mutex);
	if(Slot[(int)(unsigned char)slot_ID] != NULL){
		Slot[(int)(unsigned char)slot_ID]->Speed_Calculator.update(bytes);
	}
}

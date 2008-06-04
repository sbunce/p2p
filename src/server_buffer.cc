#include "server_buffer.h"

server_buffer::server_buffer(
	const int & socket_FD_in,
	const std::string & client_IP_in
):
	socket_FD(socket_FD_in),
	client_IP(client_IP_in)
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

bool server_buffer::create_slot(char & slot_ID, std::string path)
{
	for(int x=0; x<256; ++x){
		if(Slot[x] == NULL){
			slot_ID = (char)x;
			Slot[x] = new std::string(path);
			return true;
		}
	}
	return false;
}

bool server_buffer::slot_valid(char slot_ID)
{
	return !(Slot[(int)slot_ID] == NULL);
}

std::string & server_buffer::path(char slot_ID)
{
	assert(Slot[(int)slot_ID] != NULL);
	return *Slot[(int)slot_ID];
}

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
}

void server_buffer::close_slot(char slot_ID)
{
	Slot[(int)(unsigned char)slot_ID].clear();
}

bool server_buffer::create_slot(char & slot_ID, std::string path)
{
	for(int x=0; x<256; ++x){
		if(Slot[x].empty()){
			slot_ID = (char)x;
			Slot[x] = path;
			return true;
		}
	}
	return false;
}

bool server_buffer::slot_valid(char slot_ID)
{
	return !Slot[(int)(unsigned char)slot_ID].empty();
}

std::string & server_buffer::path(char slot_ID)
{
	return Slot[(int)(unsigned char)slot_ID];
}

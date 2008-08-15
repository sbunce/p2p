#include "server_buffer.h"

std::map<int, server_buffer *> server_buffer::Server_Buffer;
boost::mutex server_buffer::Mutex;
volatile int server_buffer::send_pending(0);

server_buffer::server_buffer()
{
	/*
	The server_buffer is used in a std::map. If std::map::[] is ever used to
	locate a client_buffer that doesn't exist the default ctor will be called and
	the program will be killed.
	*/
	logger::debug(LOGGER_P1,"improperly constructed server_buffer");
	exit(1);
}

server_buffer::server_buffer(
	const int & socket_FD_in,
	const std::string & IP_in
):
	socket_FD(socket_FD_in),
	IP(IP_in),
	exchange_key(true)

	#ifdef CORRUPT_BLOCKS
	srand(time(NULL));
	#endif
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

bool server_buffer::create_slot(char & slot_ID, const std::string & hash, const boost::uint64_t & size, const std::string & path)
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

void server_buffer::current_uploads_priv(std::vector<upload_info> & info)
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

bool server_buffer::path(char slot_ID, std::string & path)
{
	if(Slot[(int)(unsigned char)slot_ID] != NULL){
		path = Slot[(int)(unsigned char)slot_ID]->path;
		return true;
	}else{
		return false;
	}
}

void server_buffer::update_slot_percent_complete(char slot_ID, const boost::uint64_t & block_number)
{
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
	if(Slot[(int)(unsigned char)slot_ID] != NULL){
		Slot[(int)(unsigned char)slot_ID]->Speed_Calculator.update(bytes);
	}
}

void server_buffer::close_slot(server_buffer * SB)
{
	logger::debug(LOGGER_P1,"closing slot ",(int)(unsigned char)SB->recv_buff[1], " for ",SB->IP);
	SB->close_slot(SB->recv_buff[1]);
}

void server_buffer::request_slot_hash(server_buffer * SB, std::string & send)
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

void server_buffer::request_slot_file(server_buffer * SB, std::string & send)
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

void server_buffer::send_block(server_buffer * SB, std::string & send)
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

#ifndef H_SERVER_PROTOCOL
#define H_SERVER_PROTOCOL

//custom
#include "convert.h"
#include "DB_blacklist.h"
#include "DB_share.h"
#include "global.h"
#include "server_buffer.h"

//std
#include <fstream>

class server_protocol
{
public:
	server_protocol();

	/*
	process - every time data is put in to a server_buffer it should be passed to this function
	*/
	void process(server_buffer * SB, char * recv_buff, const int & n_bytes);

private:

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

	DB_share DB_Share;
};
#endif

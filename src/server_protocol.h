#ifndef H_SERVER_PROTOCOL
#define H_SERVER_PROTOCOL

//custom
#include "convert.h"
#include "DB_share.h"
#include "global.h"
#include "hex.h"
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
	void process(server_buffer * SB);

private:

	//buffer for reading file blocks from the HDD
	char send_block_buff[global::P_BLOCK_SIZE - 1];

	/*
	These functions correspond to the protocol command names. Documentation for
	what they do can be found in the protocol documentation.
	*/
	void close_slot(server_buffer * SB);
	void request_slot_hash(server_buffer * SB);
	void request_slot_file(server_buffer * SB);
	void send_block(server_buffer * SB);

	convert<uint64_t> Convert_uint64;
	DB_share DB_Share;
};
#endif

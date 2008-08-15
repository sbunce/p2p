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

};
#endif

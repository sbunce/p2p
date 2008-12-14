#ifndef H_SLOT
#define H_SLOT

//custom
#include "client_server_bridge.h"
#include "DB_blacklist.h"
#include "global.h"
#include "speed_calculator.h"
#include "upload_info.h"

class slot
{
public:
	slot();

	/*
	send_block  - answers a send block request
	              request: the P_BLOCK request
	              send: the response that is prepared in this function
	upload_info - push_back upload_info about this slot
	*/
	virtual void send_block(const std::string & request, std::string & send) = 0;
	virtual void info(std::vector<upload_info> & UI) = 0;

protected:
	std::string * IP;          //pointer to IP string which exists in server_buffer
	std::string root_hash;     //root hash (hex) of hash tree
	boost::uint64_t file_size; //size of file hash tree is for (bytes)

	//keeps track of speed for specific server
	speed_calculator Speed_Calculator;
};
#endif

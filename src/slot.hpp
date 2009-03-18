//NOT-THREADSAFE
/*
This object is used within the server_buffer which is locked.
*/
#ifndef H_SLOT
#define H_SLOT

//custom
#include "block_arbiter.hpp"
#include "database.hpp"
#include "settings.hpp"
#include "upload_info.hpp"

//include
#include <speed_calculator.hpp>

class slot
{
public:
	slot();

	/*
	get_root_hash - returns hash this slot is for
	*/
	const std::string & get_hash();

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

#ifndef H_DOWNLOAD_HASH_TREE_CONN
#define H_DOWNLOAD_HASH_TREE_CONN

//std
#include <deque>
#include <set>
#include <string>

//custom
#include "download.h"
#include "download_conn.h"
#include "speed_calculator.h"

class download_hash_tree_conn : public download_conn
{
public:
	download_hash_tree_conn(download * download_in, const std::string & IP_in);

	char slot_ID;           //slot ID the server gave for the file
	bool slot_ID_requested; //true if slot_ID requested
	bool slot_ID_received;  //true if slot_ID received

	/*
	The download will not be finished until a P_CLOSE_SLOT has been sent to all
	servers.
	*/
	bool close_slot_sent;

	//the latest file block requested from this server
	std::deque<uint64_t> latest_request;

	/*
	This stores all the blocks that have been requested from the server. When the
	every block is downloaded there will be a hash check. If any block from this
	server fails a hash check all blocks later than the bad block requested from
	this server will be re_requested.

	The purpose of re_requesting before checking is to maximize the speed of the
	download. In order to re_request sequentially the block would have be be
	re_requested from one server and all others would have to wait until it was
	known that the replacement block was good.
	*/
	std::set<uint64_t> received_blocks;

	speed_calculator Speed_Calculator;
};
#endif

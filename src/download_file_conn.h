#ifndef H_DOWNLOAD_FILE_CONN
#define H_DOWNLOAD_FILE_CONN

//std
#include <deque>
#include <string>

//custom
#include "download.h"
#include "download_conn.h"
#include "speed_calculator.h"

class download_file_conn : public download_conn
{
public:
	download_file_conn(download * download_in, const std::string & server_IP_in);

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

	speed_calculator Speed_Calculator;
};
#endif

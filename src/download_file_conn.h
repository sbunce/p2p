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
	download_file_conn(download * download_in, const std::string & server_IP_in, const unsigned int & file_ID_in);

	//the file_ID on the server (different servers have different ID for same file)
	unsigned int file_ID;

	//the latest file block requested from this server
	std::deque<unsigned int> latest_request;

	speed_calculator Speed_Calculator;
};
#endif

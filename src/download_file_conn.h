#ifndef H_DOWNLOAD_FILE_CONN
#define H_DOWNLOAD_FILE_CONN

#include <deque>
#include <string>

#include "download.h"
#include "download_conn.h"

class download_file_conn : public download_conn
{
public:
	download_file_conn(download * download_in, std::string server_IP_in, unsigned int file_ID_in);

	//the file_ID on the server (different servers have different ID for same file)
	unsigned int file_ID;

	//the latest file block requested from this server
	std::deque<unsigned int> latest_request;

	//speed of the individual server(bytes per second)
	unsigned int download_speed;

	/*
	calculate_speed - calculates the download speed for one server, should be called by response()
	speed           - returns the speed of this server(bytes per second)
	*/
	void calculate_speed(const unsigned int & packet_size);
	const unsigned int & speed();

private:
	//pair<second, bytes in second>
	std::deque<std::pair<unsigned int, unsigned int> > Download_Speed;
};
#endif

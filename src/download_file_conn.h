#ifndef H_DOWNLOAD_FILE_CONN
#define H_DOWNLOAD_FILE_CONN

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
	unsigned int latest_request;
};
#endif

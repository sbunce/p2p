#ifndef H_DOWNLOAD_CONN
#define H_DOWNLOAD_CONN

//custom
#include "download.h"

//std
#include <string>

class download; //circular dependency

class download_conn
{
public:
	download_conn();

	//socket and server_IP this download_conn is associated with
	int socket_FD;
	std::string server_IP;

	download * Download;
};
#endif

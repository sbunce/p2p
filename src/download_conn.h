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

	int socket_FD;
	std::string server_IP;
	download * Download;
};
#endif

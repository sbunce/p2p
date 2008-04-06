#ifndef H_DOWNLOAD_CONN
#define H_DOWNLOAD_CONN

//std
#include <string>

//custom
#include "download.h"

/*
Forward declaration for download. A circular dependency exists between download
and download_conn.
*/
class download;

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

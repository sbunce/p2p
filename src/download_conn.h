#ifndef H_DOWNLOAD_CONN
#define H_DOWNLOAD_CONN

//std
#include <string>

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

	//true if connect() is in progress, only used during connection phase
	bool processing;

	//holds the IP of the server to connect to, generally set in derived class ctor
	std::string server_IP;

	//the download this connection is associated with, generally set in derived class ctor
	download * Download;
};
#endif

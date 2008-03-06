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

	bool processing;       //only used during initial connection phase
	std::string server_IP; //must be present in all derived classes
	download * Download;   //the download this connection is associated with
};
#endif

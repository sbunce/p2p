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
	virtual ~download_conn();

	int socket;
	std::string IP;
	download * Download;
};
#endif

#ifndef H_DOWNLOAD_CONNECTION
#define H_DOWNLOAD_CONNECTION

//custom
#include "download.hpp"

//std
#include <string>

//needed for circular dependency
class download;

class download_connection
{
public:
	download_connection(
		download * Download_in,
		const std::string & IP_in
	):
		Download(Download_in),
		IP(IP_in),
		socket(-1)
	{}

	int socket;
	std::string IP;
	download * Download;
};
#endif

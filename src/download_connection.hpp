#ifndef H_DOWNLOAD_CONNECTION
#define H_DOWNLOAD_CONNECTION

//boost
#include <boost/shared_ptr.hpp>

//std
#include <string>

//needed for circular dependency
class download;

class download_connection
{
public:
	download_connection(
		boost::shared_ptr<download> Download_in,
		const std::string & IP_in
	):
		Download(Download_in),
		IP(IP_in),
		socket(-1)
	{}

	int socket;
	std::string IP;
	boost::shared_ptr<download> Download;
};
#endif

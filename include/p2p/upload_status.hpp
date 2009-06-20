#ifndef H_UPLOAD_STATUS
#define H_UPLOAD_STATUS

//boost
#include <boost/cstdint.hpp>

//std
#include <string>

class upload_status
{
public:
	std::string hash;
	std::string IP;            //IP of client
	boost::uint64_t size;      //file size (bytes)
	std::string path;          //empty if uploading hash tree
	unsigned percent_complete; //0 to 100
	unsigned speed;            //upload speed (bytes/second)
};
#endif

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
	std::string path;          //if hash this is empty, if file this is full path
	unsigned percent_complete; //0 to 100
	unsigned speed;            //upload speed (bytes/second)
};
#endif

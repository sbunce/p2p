#ifndef H_UPLOAD_INFO
#define H_UPLOAD_INFO

//custom
#include "global.hpp"

class upload_info
{
public:
	upload_info(
		const std::string & hash_in,
		const std::string & IP_in,
		const boost::uint64_t & size_in,
		const std::string & path_in,
		const int & speed_in,
		const int & percent_complete_in
	):
		hash(hash_in),
		IP(IP_in),
		size(size_in),
		path(path_in),
		speed(speed_in),
		percent_complete(percent_complete_in)
	{}

	std::string hash;
	std::string IP;       //IP of client
	boost::uint64_t size; //file size in bytes
	std::string path;     //if hash this is empty, if file this is full path
	int speed;            //upload speed (bytes/second)
	int percent_complete; //0 to 100
};
#endif

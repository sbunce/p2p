#ifndef H_DOWNLOAD_INFO
#define H_DOWNLOAD_INFO

//custom
#include "global.h"

//std
#include <vector>

class download_info
{
public:
	download_info(
		const std::string & hash_in,
		const std::string & name_in,
		const uint64_t & size_in,
		const int speed_in,
		const int & percent_complete_in
	):
		hash(hash_in),
		name(name_in),
		size(size_in),
		speed(speed_in),
		percent_complete(percent_complete_in)
	{}

	std::string hash;        //root hash of hash tree
	std::string name;        //name of the file
	uint64_t size;           //size of the file (bytes)
	uint64_t latest_request; //what block was most recently requested

	//information unique to the server
	std::vector<std::string> IP;

	//these are set when the download is running
	int percent_complete;
	int speed;
};
#endif

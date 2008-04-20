/*
Holds all information needed to start a download. Many classes depend on this.
*/

#ifndef H_DOWNLOAD_INFO
#define H_DOWNLOAD_INFO

//custom
#include "global.h"

class download_info
{
public:
	//ctor for starting a download
	download_info(const bool & resumed_in, const std::string & hash_in,
		const std::string & name_in, const uint64_t & size_in,
		const uint64_t & latest_request_in)
	: resumed(resumed_in), hash(hash_in), name(name_in), size(size_in),
	latest_request(latest_request_in)
	{}

	//ctor for current download information
	download_info(const std::string & hash_in,
		const std::string & name_in, const uint64_t & size_in,
		const int speed_in, const int & percent_complete_in)
	: hash(hash_in), name(name_in), size(size_in), speed(speed_in),
	percent_complete(percent_complete_in)
	{}

	bool resumed;            //true if download resumed
	std::string hash;        //root hash of hash tree, unique identifier
	std::string name;        //name of the file (ex: file.avi)
	uint64_t size;           //size of the file (bytes)
	uint64_t latest_request; //what block was most recently requested

	//information unique to the server, these vectors are parallel
	std::vector<std::string> server_IP;
	std::vector<std::string> file_ID;

	//these are set when the download is running
	int percent_complete;
	int speed;

private:
	//this enforces proper construction
	download_info();
};
#endif

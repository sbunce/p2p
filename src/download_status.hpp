/*
Status for a running download.
*/
#ifndef H_DOWNLOAD_STATUS
#define H_DOWNLOAD_STATUS

//custom
#include "settings.hpp"

//std
#include <vector>

class download_status
{
public:
	download_status(
		const std::string & hash_in,
		const std::string & name_in,
		const boost::uint64_t & size_in,
		const unsigned & total_speed_in,
		const unsigned & percent_complete_in
	):
		hash(hash_in),
		name(name_in),
		size(size_in),
		total_speed(total_speed_in),
		percent_complete(percent_complete_in)
	{}

	//information guaranteed to be set
	std::vector<std::string> IP; //IP's off all servers downloading file from
	std::string hash;            //root hash of hash tree
	std::string name;            //name of the file
	boost::uint64_t size;        //size of the file (bytes)

	//these are only set when returning information about running downloads
	unsigned percent_complete;   //0-100
	unsigned total_speed;        //total download speed (bytes/second)
	std::vector<unsigned> speed; //speed of servers (parallel to IP vector)
};
#endif

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
		const boost::uint64_t & size_in,
		const int total_speed_in = 0,
		const int percent_complete_in = 0
	):
		hash(hash_in),
		name(name_in),
		size(size_in),
		total_speed(total_speed_in),
		percent_complete(percent_complete_in)
	{}

/*
DEBUG

Think about breaking up this class in to two. download_status to view the current
status of a running download, and download_info for info to start a download.
*/

	//information guaranteed to be set
	std::vector<std::string> IP; //IP's off all servers downloading file from
	std::string hash;            //root hash of hash tree
	std::string name;            //name of the file
	boost::uint64_t size;        //size of the file (bytes)

	//these are only set when returning information about running downloads
	int percent_complete;        //0-100
	int total_speed;             //total download speed (bytes/second)
	std::vector<unsigned> speed; //speed of servers (parallel to IP vector)

	//rowid in the hash table (set by download_factory when adding download)
	boost::int64_t key;
};
#endif

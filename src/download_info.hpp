/*
Info necessary to start a download.
*/
#ifndef H_DOWNLOAD_INFO
#define H_DOWNLOAD_INFO

//custom
#include "global.hpp"

//std
#include <vector>

class download_info
{
public:
	download_info(
		const std::string & hash_in,
		const std::string & name_in,
		const boost::uint64_t & size_in
	):
		hash(hash_in),
		name(name_in),
		size(size_in)
	{}

	//information guaranteed to be set
	std::vector<std::string> IP; //IP's off all servers downloading file from
	std::string hash;            //root hash of hash tree
	std::string name;            //name of the file
	boost::uint64_t size;        //size of the file (bytes)
};
#endif

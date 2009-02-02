#ifndef H_DOWNLOAD_INFO
#define H_DOWNLOAD_INFO

//boost
#include <boost/filesystem.hpp>

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
	{
		namespace fs = boost::filesystem;
		fs::path path = fs::system_complete(fs::path(global::DOWNLOAD_DIRECTORY + name, fs::native));
		file_path = path.string();
	}

	std::vector<std::string> IP; //IP's of all servers that possibly have the file
	std::string hash;            //root hash of hash tree
	std::string name;            //name of the file
	std::string file_path;       //full path to downloading file
	boost::uint64_t size;        //size of the file (bytes)
};
#endif

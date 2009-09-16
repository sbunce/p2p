#ifndef H_FILE_INFO
#define H_FILE_INFO

//include
#include <boost/cstdint.hpp>

//standard
#include <string>

class file_info
{
public:
	file_info();
	file_info(
		const std::string & hash_in,
		const std::string & path_in,
		const boost::uint64_t & file_size_in
	);
	file_info(
		const file_info & FI
	);
	std::string hash;
	std::string path;
	boost::uint64_t file_size;
};
#endif

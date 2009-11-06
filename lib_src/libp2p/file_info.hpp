#ifndef H_FILE_INFO
#define H_FILE_INFO

//include
#include <boost/cstdint.hpp>
#include <database_connection.hpp>
#include <SHA1.hpp>

//standard
#include <ctime>
#include <string>

class file_info
{
public:
	file_info(){}

	file_info(
		const std::string & hash_in,
		const std::string & path_in,
		const boost::uint64_t & file_size_in,
		const std::time_t last_write_time_in
	):
		hash(hash_in),
		path(path_in),
		file_size(file_size_in),
		last_write_time(last_write_time_in)
	{}

	file_info(const file_info & FI):
		hash(FI.hash),
		path(FI.path),
		file_size(FI.file_size),
		last_write_time(FI.last_write_time)
	{}

	std::string hash;            //hash file is tracked by
	std::string path;            //full path to file
	boost::uint64_t file_size;   //size of file (bytes)
	std::time_t last_write_time; //file modification date
};
#endif
